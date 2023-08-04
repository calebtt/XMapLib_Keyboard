#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif
#include <Windows.h>
#include <Xinput.h>

#include "KeyboardCustomTypes.h"

#include <array>
#include <chrono>
#include <concepts>

namespace sds
{
	/**
	 * \brief A data structure to hold player information. A default constructed
	 * KeyboardPlayerInfo struct has default values that are usable. 
	 */
	struct KeyboardPlayerInfo final
	{
		int PlayerId{ 0 };
	};

	/**
	 * \brief Some constants that will someday be configurable. If these are used with config file loaded values,
	 * this type should do that upon default construction, with a non-configurable XML file name.
	 */
	struct KeyboardSettings final
	{
		/**
		 * \brief Delay each iteration of a polling loop, short enough to not miss information, long enough to not waste CPU cycles.
		 */
		static constexpr keyboardtypes::NanosDelay_t PollingLoopDelay{ std::chrono::milliseconds{1} };
		/**
		 * \brief Key Repeat Delay is the time delay a button has in-between activations.
		 */
		static constexpr keyboardtypes::NanosDelay_t KeyRepeatDelay{ std::chrono::microseconds{100'000} };
		/**
		 * \brief The button virtual keycodes as a flat array.
		 */
		static constexpr std::array<keyboardtypes::VirtualKey_t, 14> ButtonCodeArray
		{
			XINPUT_GAMEPAD_DPAD_UP,
			XINPUT_GAMEPAD_DPAD_DOWN,
			XINPUT_GAMEPAD_DPAD_LEFT,
			XINPUT_GAMEPAD_DPAD_RIGHT,
			XINPUT_GAMEPAD_START,
			XINPUT_GAMEPAD_BACK,
			XINPUT_GAMEPAD_LEFT_THUMB,
			XINPUT_GAMEPAD_RIGHT_THUMB,
			XINPUT_GAMEPAD_LEFT_SHOULDER,
			XINPUT_GAMEPAD_RIGHT_SHOULDER,
			XINPUT_GAMEPAD_A,
			XINPUT_GAMEPAD_B,
			XINPUT_GAMEPAD_X,
			XINPUT_GAMEPAD_Y
		};

		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickLeft{VK_GAMEPAD_LEFT_THUMBSTICK_LEFT};
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickRight{VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT};
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickUp{VK_GAMEPAD_LEFT_THUMBSTICK_UP};
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickDown{VK_GAMEPAD_LEFT_THUMBSTICK_DOWN};

		static constexpr keyboardtypes::VirtualKey_t RightThumbstickLeft{VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT};
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickRight{VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT};
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickUp{VK_GAMEPAD_RIGHT_THUMBSTICK_UP};
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickDown{VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN};

		// Also an internal representation value.
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickUpRight{VK_PAD_RTHUMB_UPRIGHT};
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickUpLeft{VK_PAD_RTHUMB_UPLEFT};
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickDownRight{VK_PAD_RTHUMB_DOWNRIGHT};
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickDownLeft{VK_PAD_RTHUMB_DOWNLEFT};


		// used internally to denote left or right triggers, similar to the button VKs though they may
		// not be used by the OS API state updates in the same way--we virtualize them.
		static constexpr keyboardtypes::VirtualKey_t LeftTriggerVk{VK_GAMEPAD_LEFT_TRIGGER};
		static constexpr keyboardtypes::VirtualKey_t RightTriggerVk{VK_GAMEPAD_RIGHT_TRIGGER};

		static constexpr keyboardtypes::ThumbstickValue_t LeftStickDeadzone{XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE};
		static constexpr keyboardtypes::ThumbstickValue_t RightStickDeadzone{XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE};

		static constexpr keyboardtypes::TriggerValue_t LeftTriggerThreshold{XINPUT_GAMEPAD_TRIGGER_THRESHOLD};
		static constexpr keyboardtypes::TriggerValue_t RightTriggerThreshold{XINPUT_GAMEPAD_TRIGGER_THRESHOLD};
		// The type of the button buffer without const/volatile/reference.
		using ButtonBuffer_t = std::remove_reference_t< std::remove_cv_t<decltype(ButtonCodeArray)> >;

		// TODO update this if the values become non-const
		friend auto hash_value([[maybe_unused]] const KeyboardSettings& obj) -> std::size_t { return 0x0ED35098; }
	};

	static_assert(std::copyable<KeyboardSettings>);

	/**
	 * \brief For no other reason but to make the common task of injecting these down the architecture less verbose.
	 */
	struct KeyboardSettingsPack final
	{
		KeyboardPlayerInfo playerInfo;
		KeyboardSettings settings;
	};
}