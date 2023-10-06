#pragma once
#include "pch.h"
#include "../XMapLib_Keyboard/KeyboardLegacyApiFunctions.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
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

        const auto GetBuiltMapForKeyNamed = [&](const std::string& keyName, const auto virtualKey, const int exGroup)
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
                .DelayBeforeFirstRepeat = 0ns,
                .DelayForRepeats = 0ns
            };
        };

        vector mapBuffer
        {
            // Pad buttons
            GetBuiltMapForKeyNamed("[PAD_A]", VirtualButtons::A, PadButtonsGroup),
            GetBuiltMapForKeyNamed("[PAD_B]", VirtualButtons::B, PadButtonsGroup),
            GetBuiltMapForKeyNamed("[PAD_X]", VirtualButtons::X, PadButtonsGroup),
            GetBuiltMapForKeyNamed("[PAD_Y]", VirtualButtons::Y, PadButtonsGroup),
            // Left thumbstick directional stuff
            GetBuiltMapForKeyNamed("[LTHUMB_UP]", VirtualButtons::LeftThumbstickUp, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_DOWN]", VirtualButtons::LeftThumbstickDown, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_RIGHT]", VirtualButtons::LeftThumbstickRight, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_LEFT]", VirtualButtons::LeftThumbstickLeft, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_DOWN_RIGHT]", VirtualButtons::LeftThumbstickDownRight, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_DOWN_LEFT]", VirtualButtons::LeftThumbstickDownLeft, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_UP_RIGHT]", VirtualButtons::LeftThumbstickUpRight, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTHUMB_UP_LEFT]", VirtualButtons::LeftThumbstickUpLeft, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[LTRIGGER]", VirtualButtons::LeftTrigger, LeftThumbGroup),
            GetBuiltMapForKeyNamed("[RTRIGGER]", VirtualButtons::RightTrigger, LeftThumbGroup),
            // Shoulder buttons
            CBActionMap
        	{
                .ButtonVirtualKeycode = VirtualButtons::ShoulderRight,
                .UsesInfiniteRepeat = false,
                .OnDown = []() { system("cls"); std::cout << "Cleared.\n"; }
            },
            CBActionMap
        	{
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

    inline
    auto GetMapping(const sds::VirtualButtons newVk, std::optional<int> exGroup = {})
    {
        using namespace sds;
        const std::string vkString = "Vk:[" + std::to_string((int)newVk) + "]\n";
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