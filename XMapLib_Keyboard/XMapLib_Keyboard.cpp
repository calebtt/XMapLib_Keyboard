// XMapLib_Keyboard.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KeyboardTranslationHelpers.h"
#include "ControllerButtonToActionMap.h"
#include "KeyboardTranslator.h"
#include "KeyboardLegacyApiFunctions.h"
#include "KeyboardOvertakingFilter.h"
#include "KeyboardSFMLController.h"

#include "../XMapLib_Utils/nanotime.h"
#include "../XMapLib_Utils/SendMouseInput.h"
#include "../XMapLib_Utils/ControllerStatus.h"

#include <iostream>
#include <chrono>
#include <format>

// Crude mechanism to keep the loop running until [enter] is pressed.
struct GetterExitCallable final
{
    std::atomic<bool> IsDone{ false };
    void GetExitSignal()
    {
        std::string buf;
        std::getline(std::cin, buf);
        IsDone.store(true, std::memory_order_relaxed);
    }
};

auto GetEpochTimestamp()
{
    const auto currentTime = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch());
}

auto GetDriverButtonMappings()
{
    using std::vector, sds::CBActionMap, std::cout;
    using namespace std::chrono_literals;
    using namespace sds;

    constexpr int PadButtonsGroup = 111; // Buttons exclusivity grouping.
    constexpr int LeftThumbGroup = 101; // Left thumbstick exclusivity grouping.
    const auto PrintMessageAndTime = [](std::string_view msg)
    {
        cout << msg << " @" << GetEpochTimestamp() << '\n';
    };
    const auto GetDownLambdaForKeyNamed = [=](const std::string& keyName)
	{
        return [=]() { PrintMessageAndTime(keyName + "=[DOWN]"); };
    };
    const auto GetUpLambdaForKeyNamed = [=](const std::string& keyName)
    {
        return [=]() { PrintMessageAndTime(keyName + "=[UP]"); };
    };
    const auto GetRepeatLambdaForKeyNamed = [=](const std::string& keyName)
    {
        return [=]() { PrintMessageAndTime(keyName + "=[REPEAT]"); };
    };
    const auto GetResetLambdaForKeyNamed = [=](const std::string& keyName)
    {
        return [=]() { PrintMessageAndTime(keyName + "=[RESET]"); };
    };

    const auto GetBuiltMapForKeyNamed = [&](const std::string & keyName, const auto virtualKey, const int exGroup, const auto firstDelay)
    {
        return CBActionMap
        {
            .ButtonVirtualKeycode = virtualKey,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = exGroup,
            .OnDown = GetDownLambdaForKeyNamed(keyName),
            .OnUp = GetUpLambdaForKeyNamed(keyName),
            .OnRepeat = GetRepeatLambdaForKeyNamed(keyName),
            .OnReset = GetResetLambdaForKeyNamed(keyName),
            .DelayBeforeFirstRepeat = firstDelay
        };
    };

    vector mapBuffer
    {
        // Pad buttons
        GetBuiltMapForKeyNamed("[PAD_A]", VirtualButtons::A, PadButtonsGroup, 500ms),
        GetBuiltMapForKeyNamed("[PAD_B]", VirtualButtons::B, PadButtonsGroup, 500ms),
        GetBuiltMapForKeyNamed("[PAD_X]", VirtualButtons::X, PadButtonsGroup, 500ms),
        GetBuiltMapForKeyNamed("[PAD_Y]", VirtualButtons::Y, PadButtonsGroup, 500ms),
        // Dpad
        GetBuiltMapForKeyNamed("[DPAD_LEFT]", VirtualButtons::DpadLeft, {}, 500ms),
        GetBuiltMapForKeyNamed("[DPAD_RIGHT]", VirtualButtons::DpadRight, {}, 500ms),
        GetBuiltMapForKeyNamed("[DPAD_UP]", VirtualButtons::DpadUp, {}, 500ms),
        GetBuiltMapForKeyNamed("[DPAD_DOWN]", VirtualButtons::DpadDown, {}, 500ms),
        // Left thumbstick directional stuff
        GetBuiltMapForKeyNamed("[LTHUMB_UP]", VirtualButtons::LeftThumbstickUp, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_DOWN]", VirtualButtons::LeftThumbstickDown, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_RIGHT]", VirtualButtons::LeftThumbstickRight, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_LEFT]", VirtualButtons::LeftThumbstickLeft, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_DOWN_RIGHT]", VirtualButtons::LeftThumbstickDownRight, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_DOWN_LEFT]", VirtualButtons::LeftThumbstickDownLeft, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_UP_RIGHT]", VirtualButtons::LeftThumbstickUpRight, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTHUMB_UP_LEFT]", VirtualButtons::LeftThumbstickUpLeft, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[LTRIGGER]", VirtualButtons::LeftTrigger, LeftThumbGroup, 500ms),
        GetBuiltMapForKeyNamed("[RTRIGGER]", VirtualButtons::RightTrigger, LeftThumbGroup, 500ms),
        // Shoulder buttons
        CBActionMap{
            .ButtonVirtualKeycode = VirtualButtons::ShoulderRight,
            .UsesInfiniteRepeat = false,
            .OnDown = []() { system("cls"); std::cout << "Cleared.\n"; }
        },
    	CBActionMap{
            .ButtonVirtualKeycode = VirtualButtons::ShoulderLeft,
            .UsesInfiniteRepeat = false,
            .OnDown = []()
            {
                // Add impl for something to do here
            }
        },
    };

    return mapBuffer;
}

auto GetDriverMouseMappings()
{
    using std::vector, std::cout;
    using namespace std::chrono_literals;
    using namespace sds;
    using enum sds::VirtualButtons;

    constexpr auto FirstDelay = 0ns; // mouse move delays
    constexpr auto RepeatDelay = 1200us;
    constexpr int MouseExGroup = 102;

    vector mapBuffer
    {
        // Mouse move stuff
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickUp,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(0, 1);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(0, 1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickUpRight,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(1, 1);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(1, 1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickUpLeft,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(-1, 1);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(-1, 1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickDown,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(0, -1);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(0, -1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickLeft,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(-1, 0);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(-1, 0);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickRight,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(1, 0);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(1, 0);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickDownRight,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(1, -1);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(1, -1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickDownLeft,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = []()
            {
                Utilities::SendMouseMove(-1, -1);
            },
            .OnRepeat = []()
            {
                Utilities::SendMouseMove(-1, -1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
    };

	return mapBuffer;
}

inline
void TranslationLoopXbox(const sds::XInput::KeyboardSettingsXInput& settingsPack, sds::KeyboardTranslator<>& translator, const std::chrono::nanoseconds sleepDelay)
{
    using namespace std::chrono_literals;
	const auto translation = translator.GetUpdatedState(sds::XInput::GetWrappedControllerStateUpdate(settingsPack));
	translation();
	nanotime_sleep(sleepDelay.count());
}

inline
void TranslationLoopPs5(const sds::PS5::KeyboardSettingsSFMLPlayStation5& settingsPack, sds::KeyboardTranslator<>& translator, const std::chrono::nanoseconds sleepDelay)
{
    using namespace std::chrono_literals;
    const auto translation = translator.GetUpdatedState(sds::PS5::GetWrappedControllerStateUpdate(settingsPack));
    translation();
    nanotime_sleep(sleepDelay.count());
}

auto RunTestDriverLoop()
{
    using namespace std::chrono_literals;

    // Building mappings buffer
    auto mapBuffer = GetDriverButtonMappings();
    mapBuffer.append_range(GetDriverMouseMappings());

    std::cout << std::vformat("Created mappings buffer with {} mappings. Total size: {} bytes.\n", std::make_format_args(mapBuffer.size(), sizeof(mapBuffer.front())*mapBuffer.size()));

    // Creating a few polling/translation related types
    constexpr sds::XInput::KeyboardSettingsXInput xboxSettings{};
    constexpr sds::PS5::KeyboardSettingsSFMLPlayStation5 ps5Settings{};
    // The filter is constructed here, to support custom filters with their own construction needs.
    sds::KeyboardOvertakingFilter filter{};
    // Filter is then moved into the translator at construction.
    sds::KeyboardTranslator translator{ std::move(mapBuffer), std::move(filter) };

    constexpr auto SleepDelay = std::chrono::nanoseconds{ 500us };
    static constexpr std::size_t IterationsForTiming{ 10'000 };
    std::size_t iterationCount{};
    auto startTime{ std::chrono::steady_clock::now() };
    const auto updateLoopTimer = [&startTime, &iterationCount](const std::chrono::nanoseconds sleepDelay)
    {
        ++iterationCount;
        if(iterationCount >= IterationsForTiming)
        {
            const auto endTime = std::chrono::steady_clock::now();
            const auto diffTime = endTime - startTime;
            const auto timePerIter = std::chrono::duration_cast<std::chrono::microseconds>(diffTime / IterationsForTiming - sleepDelay);
            const auto millisPerIter = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(timePerIter);
            std::cout << "Average time over " << IterationsForTiming << " iterations, per iteration: " << timePerIter << " -or- " << millisPerIter << '\n';

            iterationCount = {};
            startTime = std::chrono::steady_clock::now();
        }
    };

    GetterExitCallable gec;
    const auto exitFuture = std::async(std::launch::async, [&]() { gec.GetExitSignal(); });
    while (!gec.IsDone)
    {
        //TranslationLoopPs5(ps5Settings, translator, SleepDelay);
        TranslationLoopXbox(xboxSettings, translator, SleepDelay);
        updateLoopTimer(SleepDelay);
    }
    std::cout << "Performing cleanup actions...\n";
    const auto cleanupTranslations = translator.GetCleanupActions();
    for (auto& cleanupAction : cleanupTranslations)
        cleanupAction();

    exitFuture.wait();
}


// Test driver program for keyboard mapping
int main()
{
    RunTestDriverLoop();
}