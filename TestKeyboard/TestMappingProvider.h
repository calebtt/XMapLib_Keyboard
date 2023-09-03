#pragma once
#include "pch.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
    /**
     * \brief A,B,X,Y Buttons are all in ex. group 111, Left thumbstick directions all in group 101.
     */
    inline
    auto GetDriverButtonMappings()
    {
        using std::vector, sds::CBActionMap, std::cout;
        using namespace std::chrono_literals;
        using namespace sds;

    	static constexpr KeyboardSettings ksp;
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
                .ButtonVirtualKeycode = ksp.ButtonA,
                .UsesInfiniteRepeat = true,
                .ExclusivityGrouping = PadButtonsGroup,
                .OnDown = GetDownLambdaForKeyNamed("[PAD_A]"),
                .OnUp = GetUpLambdaForKeyNamed("[PAD_A]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_A]")
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.ButtonB,
                .UsesInfiniteRepeat = false,
                .SendsFirstRepeatOnly = true,
                .ExclusivityGrouping = PadButtonsGroup,
                .OnDown = GetDownLambdaForKeyNamed("[PAD_B]"),
                .OnUp = GetUpLambdaForKeyNamed("[PAD_B]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_B]"),
                .OnReset = GetResetLambdaForKeyNamed("[PAD_B]")
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.ButtonX,
                .UsesInfiniteRepeat = false,
                .SendsFirstRepeatOnly = true,
                .ExclusivityGrouping = PadButtonsGroup,
                .OnDown = GetDownLambdaForKeyNamed("[PAD_X]"),
                .OnUp = GetUpLambdaForKeyNamed("[PAD_X]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_X]"),
                .OnReset = GetResetLambdaForKeyNamed("[PAD_X]")
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.ButtonY,
                .UsesInfiniteRepeat = false,
                .SendsFirstRepeatOnly = true,
                .ExclusivityGrouping = PadButtonsGroup,
                .OnDown = GetDownLambdaForKeyNamed("[PAD_Y]"),
                .OnUp = GetUpLambdaForKeyNamed("[PAD_Y]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[PAD_Y]"),
                .OnReset = GetResetLambdaForKeyNamed("[PAD_Y]")
            },
            // Left thumbstick directional stuff
            CBActionMap{
                .ButtonVirtualKeycode = ksp.LeftThumbstickUp,
                .UsesInfiniteRepeat = true,
                .ExclusivityGrouping = LeftThumbGroup,
                .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_UP]"),
                .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_UP]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_UP]"),
                .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_UP]"),
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.LeftThumbstickDown,
                .UsesInfiniteRepeat = true,
                .ExclusivityGrouping = LeftThumbGroup,
                .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_DOWN]"),
                .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_DOWN]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_DOWN]"),
                .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_DOWN]"),
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.LeftThumbstickRight,
                .UsesInfiniteRepeat = true,
                .ExclusivityGrouping = LeftThumbGroup,
                .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_RIGHT]"),
                .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_RIGHT]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_RIGHT]"),
                .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_RIGHT]"),
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.LeftThumbstickLeft,
                .UsesInfiniteRepeat = true,
                .ExclusivityGrouping = LeftThumbGroup,
                .OnDown = GetDownLambdaForKeyNamed("[LTHUMB_LEFT]"),
                .OnUp = GetUpLambdaForKeyNamed("[LTHUMB_LEFT]"),
                .OnRepeat = GetRepeatLambdaForKeyNamed("[LTHUMB_LEFT]"),
                .OnReset = GetResetLambdaForKeyNamed("[LTHUMB_LEFT]"),
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.LeftTrigger,
                .UsesInfiniteRepeat = false,
                .ExclusivityGrouping = LeftThumbGroup,
                .OnDown = GetDownLambdaForKeyNamed("[LTRIGGER]"),
                .OnUp = GetUpLambdaForKeyNamed("[LTRIGGER]"),
                .OnReset = GetResetLambdaForKeyNamed("[LTRIGGER]"),
                .DelayBeforeFirstRepeat = 1ns,
                .DelayForRepeats = 1ns
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.RightTrigger,
                .UsesInfiniteRepeat = false,
                .ExclusivityGrouping = LeftThumbGroup,
                .OnDown = GetDownLambdaForKeyNamed("[RTRIGGER]"),
                .OnUp = GetUpLambdaForKeyNamed("[RTRIGGER]"),
                .OnReset = GetResetLambdaForKeyNamed("[RTRIGGER]"),
                .DelayBeforeFirstRepeat = 1ns,
                .DelayForRepeats = 1ns
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.ButtonShoulderRight,
                .UsesInfiniteRepeat = false,
                .OnDown = [=]() { PrintMessageAndTime("Cleared.\n"); }
            },
            CBActionMap{
                .ButtonVirtualKeycode = ksp.ButtonShoulderLeft,
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