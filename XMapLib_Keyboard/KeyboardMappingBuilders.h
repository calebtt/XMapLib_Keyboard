#pragma once
#include "ControllerButtonToActionMap.h"

#include <string>
#include <chrono>
#include <functional>
#include <concepts>
#include <type_traits>

namespace sds
{
	/**
     * \brief  A possibly useful function to return a mapping. Infinite repeat mode, no custom repeat delay except for first repeat.
     * \param virtualKey    Virtual key integral value denoting which controller button/stick/trigger this mapping refers to. See: KeyboardSettings.h
     * \param onDown  Function produced when a key moves to the 'down' state.
     * \param onUp  Function produced when a key moves to the 'up' state.
     * \param onRepeat  Function produced when a key moves to the 'repeat' state.
     * \param onReset   Function produced when a key moves to the 'reset' state (ready for another cycle).
     * \param exGroup   Exclusivity grouping integral value, the default (included) behavior for ex. group mappings will only allow one mapping to be 'down' at a time,
     *  and activating a mapping with the same ex. group will make it overtake the currently activated mapping. Mapping that have been overtaken, but their controller button still pressed,
     *  will be added to a queue and only moved back to activation when the button before it in the queue (most recently activated) has been released.
     * \param delayBeforeFirstRepeat    This delay is used after an initial key-down, before the first time it starts repeating.
     * \return  The built mapping.
     */
    inline
    auto GetBuiltMappingInfiniteRepeatNoCustomDelay(
        const keyboardtypes::VirtualKey_t virtualKey,
        std::function<void()> onDown = {},
        std::function<void()> onUp = {},
        std::function<void()> onRepeat = {},
        std::function<void()> onReset = {},
        std::optional<int> exGroup = {},
        std::chrono::nanoseconds delayBeforeFirstRepeat = std::chrono::microseconds{ 100'000 })
    {
        return CBActionMap
        {
            .ButtonVirtualKeycode = virtualKey,
            .UsesInfiniteRepeat = true,
            .ExclusivityGrouping = exGroup,
            .OnDown = std::move(onDown),
            .OnUp = std::move(onUp),
            .OnRepeat = std::move(onRepeat),
            .OnReset = std::move(onReset),
            .DelayBeforeFirstRepeat = delayBeforeFirstRepeat
        };
    }
}
