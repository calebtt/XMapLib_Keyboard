// XMapLib_Keyboard.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "KeyboardTranslationHelpers.h"
#include "ControllerButtonToActionMap.h"
#include "KeyboardPollerController.h"
#include "KeyboardLegacyApiFunctions.h"
#include "KeyboardOvertakingFilter.h"
#include "../XMapLib_Utils/nanotime.h"
#include "../XMapLib_Utils/SendMouseInput.h"
#include "../XMapLib_Utils/ControllerStatus.h"

#include <iostream>


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
    // These are a good idea in case someone wants this to work for XINPUT_KEYSTROKE instead, just change these to the VK_ ones.
    constexpr keyboardtypes::VirtualKey_t ButtonA{XINPUT_GAMEPAD_A};
    constexpr keyboardtypes::VirtualKey_t ButtonB{XINPUT_GAMEPAD_B};
    constexpr keyboardtypes::VirtualKey_t ButtonX{XINPUT_GAMEPAD_X};
    constexpr keyboardtypes::VirtualKey_t ButtonY{XINPUT_GAMEPAD_Y};

    constexpr keyboardtypes::VirtualKey_t ButtonStart{XINPUT_GAMEPAD_START};
    constexpr keyboardtypes::VirtualKey_t ButtonBack{XINPUT_GAMEPAD_BACK};
    constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeft{XINPUT_GAMEPAD_LEFT_SHOULDER};
    constexpr keyboardtypes::VirtualKey_t ButtonShoulderRight{XINPUT_GAMEPAD_RIGHT_SHOULDER};

	constexpr keyboardtypes::VirtualKey_t DpadUp{XINPUT_GAMEPAD_DPAD_UP};
    constexpr keyboardtypes::VirtualKey_t DpadDown{XINPUT_GAMEPAD_DPAD_DOWN};
    constexpr keyboardtypes::VirtualKey_t DpadLeft{XINPUT_GAMEPAD_DPAD_LEFT};
    constexpr keyboardtypes::VirtualKey_t DpadRight{XINPUT_GAMEPAD_DPAD_RIGHT};

    constexpr keyboardtypes::VirtualKey_t ThumbLeftClick{XINPUT_GAMEPAD_LEFT_THUMB};
    constexpr keyboardtypes::VirtualKey_t ThumbRightClick{XINPUT_GAMEPAD_RIGHT_THUMB};

    static constexpr keyboardtypes::VirtualKey_t LeftThumbstickLeft{VK_GAMEPAD_LEFT_THUMBSTICK_LEFT}; // These are represented internally with the VK defines as there is none for the old API.
    static constexpr keyboardtypes::VirtualKey_t LeftThumbstickRight{VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT};
    static constexpr keyboardtypes::VirtualKey_t LeftThumbstickUp{VK_GAMEPAD_LEFT_THUMBSTICK_UP};
    static constexpr keyboardtypes::VirtualKey_t LeftThumbstickDown{VK_GAMEPAD_LEFT_THUMBSTICK_DOWN};

    static constexpr keyboardtypes::VirtualKey_t RightThumbstickLeft{VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickRight{VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickUp{VK_GAMEPAD_RIGHT_THUMBSTICK_UP};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickDown{VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN};

    static constexpr keyboardtypes::VirtualKey_t TriggerLeft{VK_GAMEPAD_LEFT_TRIGGER};
    static constexpr keyboardtypes::VirtualKey_t TriggerRight{VK_GAMEPAD_RIGHT_TRIGGER};

    vector mapBuffer
    {
        CBActionMap{
            .ButtonVirtualKeycode = ButtonA,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = PadButtonsGroup,
            .OnDown = GetDownLambdaForKeyNamed("[PAD_A]"),
            .OnUp = GetUpLambdaForKeyNamed("[PAD_A]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_A]"),
            .DelayBeforeFirstRepeat = 500ms
        },
        CBActionMap{
            .ButtonVirtualKeycode = ButtonB,
            .UsesInfiniteRepeat = false,
            .SendsFirstRepeatOnly = true,
            .ExclusivityGrouping = PadButtonsGroup,
            .OnDown = GetDownLambdaForKeyNamed("[PAD_B]"),
            .OnUp = GetUpLambdaForKeyNamed("[PAD_B]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_B]"),
            .OnReset = GetResetLambdaForKeyNamed("[PAD_B]"),
            .DelayBeforeFirstRepeat = 2s
        },
    	CBActionMap{
            .ButtonVirtualKeycode = ButtonX,
            .UsesInfiniteRepeat = false,
            .SendsFirstRepeatOnly = true,
            .ExclusivityGrouping = PadButtonsGroup,
            .OnDown = GetDownLambdaForKeyNamed("[PAD_X]"),
            .OnUp = GetUpLambdaForKeyNamed("[PAD_X]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_X]"),
            .OnReset = GetResetLambdaForKeyNamed("[PAD_X]"),
            .DelayBeforeFirstRepeat = 2s
        },
        CBActionMap{
            .ButtonVirtualKeycode = ButtonY,
            .UsesInfiniteRepeat = false,
            .SendsFirstRepeatOnly = true,
            .ExclusivityGrouping = PadButtonsGroup,
            .OnDown = GetDownLambdaForKeyNamed("[PAD_Y]"),
            .OnUp = GetUpLambdaForKeyNamed("[PAD_Y]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_Y]"),
            .OnReset = GetResetLambdaForKeyNamed("[PAD_Y]"),
            .DelayBeforeFirstRepeat = 2s
        },
        // Left thumbstick directional stuff
        CBActionMap{
            .ButtonVirtualKeycode = LeftThumbstickUp,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = LeftThumbGroup,
            .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_UP]"),
            .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_UP]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_UP]"),
            .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_UP]"),
        },
        CBActionMap{
            .ButtonVirtualKeycode = LeftThumbstickDown,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = LeftThumbGroup,
            .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_DOWN]"),
            .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_DOWN]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_DOWN]"),
            .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_DOWN]"),
        },
        CBActionMap{
            .ButtonVirtualKeycode = LeftThumbstickRight,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = LeftThumbGroup,
            .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_RIGHT]"),
            .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_RIGHT]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_RIGHT]"),
            .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_RIGHT]"),
        },
        CBActionMap{
            .ButtonVirtualKeycode = LeftThumbstickLeft,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = LeftThumbGroup,
            .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_LEFT]"),
            .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_LEFT]"),
            .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_LEFT]"),
            .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_LEFT]"),
        },
        CBActionMap{
            .ButtonVirtualKeycode = TriggerLeft,
            .UsesInfiniteRepeat = false,
            .ExclusivityGrouping = LeftThumbGroup,
            .OnDown = GetDownLambdaForKeyNamed("[LTRIGGER]"),
            .OnUp = GetUpLambdaForKeyNamed("[LTRIGGER]"),
            .OnReset = GetResetLambdaForKeyNamed("[LTRIGGER]"),
            .DelayBeforeFirstRepeat = 1ns,
            .DelayForRepeats = 1ns
        },
    	CBActionMap{
            .ButtonVirtualKeycode = TriggerRight,
            .UsesInfiniteRepeat = false,
            .ExclusivityGrouping = LeftThumbGroup,
            .OnDown = GetDownLambdaForKeyNamed("[RTRIGGER]"),
            .OnUp = GetUpLambdaForKeyNamed("[RTRIGGER]"),
            .OnReset = GetResetLambdaForKeyNamed("[RTRIGGER]"),
            .DelayBeforeFirstRepeat = 1ns,
            .DelayForRepeats = 1ns
        },
        CBActionMap{
            .ButtonVirtualKeycode = ButtonShoulderRight,
            .UsesInfiniteRepeat = false,
            .OnDown = []() { system("cls"); std::cout << "Cleared.\n"; }
        },
    	CBActionMap{
            .ButtonVirtualKeycode = ButtonShoulderLeft,
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
    sds::Utilities::SendMouseInput smi;
    constexpr auto FirstDelay = 0ns; // mouse move delays
    constexpr auto RepeatDelay = 1200us;
    constexpr int MouseExGroup = 102;

    static constexpr keyboardtypes::VirtualKey_t RightThumbstickLeft{VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickRight{VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickUp{VK_GAMEPAD_RIGHT_THUMBSTICK_UP};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickDown{VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN};

    // Internal reprsentation of these values, not from OS API though it may use a macro from there.
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickUpRight{KeyboardSettings::RightThumbstickUpRight};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickUpLeft{KeyboardSettings::RightThumbstickUpLeft};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickDownRight{KeyboardSettings::RightThumbstickDownRight};
    static constexpr keyboardtypes::VirtualKey_t RightThumbstickDownLeft{KeyboardSettings::RightThumbstickDownLeft};

    vector mapBuffer
    {
        // Mouse move stuff
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickUp,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = [smi]() mutable
            {
                smi.SendMouseMove(0, 1);
            },
            .OnRepeat = [smi]() mutable
            {
                smi.SendMouseMove(0, 1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickDown,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = [smi]() mutable
            {
                smi.SendMouseMove(0, -1);
            },
            .OnRepeat = [smi]() mutable
            {
                smi.SendMouseMove(0, -1);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickLeft,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = [smi]() mutable
            {
                smi.SendMouseMove(-1, 0);
            },
            .OnRepeat = [smi]() mutable
            {
                smi.SendMouseMove(-1, 0);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        },
        CBActionMap{
            .ButtonVirtualKeycode = RightThumbstickRight,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = MouseExGroup,
            .OnDown = [smi]() mutable
            {
                smi.SendMouseMove(1, 0);
            },
            .OnRepeat = [smi]() mutable
            {
                smi.SendMouseMove(1, 0);
            },
            .DelayBeforeFirstRepeat = FirstDelay,
            .DelayForRepeats = RepeatDelay
        }
    };
    return mapBuffer;
}

auto RunTestDriverLoop()
{
    using namespace std::chrono_literals;

    // Building mappings buffer
    auto mapBuffer = GetDriverButtonMappings();
    mapBuffer.append_range(GetDriverMouseMappings());

    for (auto& e : mapBuffer)
        std::cout << hash_value(e) << '\n';

    // Creating a few polling/translation related types
    const sds::KeyboardPlayerInfo playerInfo{};
    sds::KeyboardOvertakingFilter filter{};
    sds::KeyboardPollerControllerLegacy poller{std::move(mapBuffer), std::move(filter) };
    GetterExitCallable gec;
    const auto exitFuture = std::async(std::launch::async, [&]() { gec.GetExitSignal(); });
    while (!gec.IsDone)
    {
        constexpr auto sleepDelay = std::chrono::nanoseconds{ 500us };
        const auto stateUpdate = sds::GetWrappedLegacyApiStateUpdate(playerInfo.PlayerId);
        const auto translation = poller(stateUpdate);
        translation();
        if(sds::ControllerStatus::IsControllerConnected(playerInfo.PlayerId))
            nanotime_sleep(sleepDelay.count());
        else
            nanotime_sleep(sleepDelay.count()*2);
    }
    std::cout << "Performing cleanup actions...\n";
    const auto cleanupTranslation = poller.GetCleanupActions();
    for (auto& cleanupAction : cleanupTranslation)
        cleanupAction();

    exitFuture.wait();
}


// Test driver program for keyboard mapping
int main()
{
    RunTestDriverLoop();
}