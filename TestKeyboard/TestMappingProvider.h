#pragma once
#include "pch.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
    // These are a good idea in case someone wants this to work for XINPUT_KEYSTROKE instead, just change these to the VK_ ones.
    constexpr sds::keyboardtypes::VirtualKey_t ButtonA{ XINPUT_GAMEPAD_A };
    constexpr sds::keyboardtypes::VirtualKey_t ButtonB{ XINPUT_GAMEPAD_B };
    constexpr sds::keyboardtypes::VirtualKey_t ButtonX{ XINPUT_GAMEPAD_X };
    constexpr sds::keyboardtypes::VirtualKey_t ButtonY{ XINPUT_GAMEPAD_Y };

    constexpr sds::keyboardtypes::VirtualKey_t ButtonStart{ XINPUT_GAMEPAD_START };
    constexpr sds::keyboardtypes::VirtualKey_t ButtonBack{ XINPUT_GAMEPAD_BACK };
    constexpr sds::keyboardtypes::VirtualKey_t ButtonShoulderLeft{ XINPUT_GAMEPAD_LEFT_SHOULDER };
    constexpr sds::keyboardtypes::VirtualKey_t ButtonShoulderRight{ XINPUT_GAMEPAD_RIGHT_SHOULDER };

    constexpr sds::keyboardtypes::VirtualKey_t DpadUp{ XINPUT_GAMEPAD_DPAD_UP };
    constexpr sds::keyboardtypes::VirtualKey_t DpadDown{ XINPUT_GAMEPAD_DPAD_DOWN };
    constexpr sds::keyboardtypes::VirtualKey_t DpadLeft{ XINPUT_GAMEPAD_DPAD_LEFT };
    constexpr sds::keyboardtypes::VirtualKey_t DpadRight{ XINPUT_GAMEPAD_DPAD_RIGHT };

    constexpr sds::keyboardtypes::VirtualKey_t ThumbLeftClick{ XINPUT_GAMEPAD_LEFT_THUMB };
    constexpr sds::keyboardtypes::VirtualKey_t ThumbRightClick{ XINPUT_GAMEPAD_RIGHT_THUMB };

    static constexpr sds::keyboardtypes::VirtualKey_t LeftThumbstickLeft{ VK_GAMEPAD_LEFT_THUMBSTICK_LEFT }; // These are represented internally with the VK defines as there is none for the old API.
    static constexpr sds::keyboardtypes::VirtualKey_t LeftThumbstickRight{ VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT };
    static constexpr sds::keyboardtypes::VirtualKey_t LeftThumbstickUp{ VK_GAMEPAD_LEFT_THUMBSTICK_UP };
    static constexpr sds::keyboardtypes::VirtualKey_t LeftThumbstickDown{ VK_GAMEPAD_LEFT_THUMBSTICK_DOWN };

    static constexpr sds::keyboardtypes::VirtualKey_t RightThumbstickLeft{ VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT };
    static constexpr sds::keyboardtypes::VirtualKey_t RightThumbstickRight{ VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT };
    static constexpr sds::keyboardtypes::VirtualKey_t RightThumbstickUp{ VK_GAMEPAD_RIGHT_THUMBSTICK_UP };
    static constexpr sds::keyboardtypes::VirtualKey_t RightThumbstickDown{ VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN };

    static constexpr sds::keyboardtypes::VirtualKey_t TriggerLeft{ VK_GAMEPAD_LEFT_TRIGGER };
    static constexpr sds::keyboardtypes::VirtualKey_t TriggerRight{ VK_GAMEPAD_RIGHT_TRIGGER };


    /**
     * \brief A,B,X,Y Buttons are all in ex. group 111, Left thumbstick directions all in group 101.
     */
    inline
    auto GetDriverButtonMappings()
    {
        using std::vector, sds::CBActionMap, std::cout;
        using namespace std::chrono_literals;
        using namespace sds;

        constexpr int PadButtonsGroup = 111; // Buttons exclusivity grouping.
        constexpr int LeftThumbGroup = 101; // Left thumbstick exclusivity grouping.
        const auto PrintMessageAndTime = [](std::string_view msg)
        {
            Logger::WriteMessage(std::vformat("{}\n", std::make_format_args(msg)).c_str());
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
                .DelayBeforeFirstRepeat = 1s
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
                .DelayBeforeFirstRepeat = 1s
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
                .DelayBeforeFirstRepeat = 1s
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
                .OnDown = [=]() { PrintMessageAndTime("Cleared.\n"); }
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

    inline
    auto GetMapping(const unsigned short newVk, std::optional<int> exGroup = {})
    {
        using namespace sds;
        const std::string vkString = "Vk:[" + std::to_string(newVk) + "]\n";
        const std::string downMessage = "Action:[Down] " + vkString;
        const std::string upMessage = "Action:[Up] " + vkString;
        const std::string repeatMessage = "Action:[Repeat] " + vkString;
        const std::string resetMessage = "Action:[Reset] " + vkString;

        std::vector<CBActionMap> mappings;
        CBActionMap tm{
            .ButtonVirtualKeycode = newVk,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = exGroup,
            .OnDown = [=]() { Logger::WriteMessage(downMessage.c_str()); },
            .OnUp = [=]() { Logger::WriteMessage(upMessage.c_str()); },
            .OnRepeat = [=]() { Logger::WriteMessage(repeatMessage.c_str()); },
            .OnReset = [=]() { Logger::WriteMessage(resetMessage.c_str()); },
            .LastAction = {}
        };
        mappings.emplace_back(tm);
        return mappings;
    }
}