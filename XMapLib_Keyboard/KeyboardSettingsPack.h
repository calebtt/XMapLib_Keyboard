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
#include <numbers>

namespace sds
{
	enum class ControllerStick
	{
		LeftStick,
		RightStick
	};

	/**
	 * \brief Direction values for thumbsticks, includes diagonals.
	 */
	enum class ThumbstickDirection : int32_t
	{
		Up,
		UpRight,
		Right,
		RightDown,
		Down,
		DownLeft,
		Left,
		LeftUp,
		Invalid
	};

	inline
	auto ThumbstickDirectionToString(const ThumbstickDirection direction) -> std::string
	{
		switch (direction)
		{
		case ThumbstickDirection::Up:
			return "Up";
		case ThumbstickDirection::UpRight:
			return "UpRight";
		case ThumbstickDirection::Right:
			return "Right";
		case ThumbstickDirection::RightDown:
			return "RightDown";
		case ThumbstickDirection::Down:
			return "Down";
		case ThumbstickDirection::DownLeft:
			return "DownLeft";
		case ThumbstickDirection::Left:
			return "Left";
		case ThumbstickDirection::LeftUp:
			return "LeftUp";
		case ThumbstickDirection::Invalid:
			return "Invalid";
		default: return "Unknown/Invalid";
		}
	}

	/**
	 * \brief A data structure to hold player information. A default constructed
	 * KeyboardPlayerInfo struct has default values that are usable. 
	 */
	struct KeyboardPlayerInfo final
	{
		int PlayerId{ 0 };
	};

	/**
	 * \brief Some constants that are not configurable.
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

		// Controller buttons
		static constexpr keyboardtypes::VirtualKey_t ButtonA{ XINPUT_GAMEPAD_A };
		static constexpr keyboardtypes::VirtualKey_t ButtonB{ XINPUT_GAMEPAD_B };
		static constexpr keyboardtypes::VirtualKey_t ButtonX{ XINPUT_GAMEPAD_X };
		static constexpr keyboardtypes::VirtualKey_t ButtonY{ XINPUT_GAMEPAD_Y };

		// Dpad buttons
		static constexpr keyboardtypes::VirtualKey_t DpadUp{ XINPUT_GAMEPAD_DPAD_UP };
		static constexpr keyboardtypes::VirtualKey_t DpadDown{ XINPUT_GAMEPAD_DPAD_DOWN };
		static constexpr keyboardtypes::VirtualKey_t DpadLeft{ XINPUT_GAMEPAD_DPAD_LEFT };
		static constexpr keyboardtypes::VirtualKey_t DpadRight{ XINPUT_GAMEPAD_DPAD_RIGHT };

		// Left thumbstick directions
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickUp{ VK_GAMEPAD_LEFT_THUMBSTICK_UP }; // UP
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickUpRight{ VK_PAD_LTHUMB_UPRIGHT }; // UP-RIGHT
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickRight{ VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT }; // RIGHT
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickDownRight{ VK_PAD_LTHUMB_DOWNRIGHT }; // RIGHT-DOWN
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickDown{ VK_GAMEPAD_LEFT_THUMBSTICK_DOWN }; // DOWN
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickDownLeft{ VK_PAD_LTHUMB_DOWNLEFT }; // DOWN-LEFT
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickLeft{VK_GAMEPAD_LEFT_THUMBSTICK_LEFT }; // LEFT
		static constexpr keyboardtypes::VirtualKey_t LeftThumbstickUpLeft{ VK_PAD_LTHUMB_UPLEFT }; // UP-LEFT

		// Right thumbstick directions
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickUp{ VK_GAMEPAD_RIGHT_THUMBSTICK_UP }; // UP
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickUpRight{ VK_PAD_RTHUMB_UPRIGHT }; //UP-RIGHT
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickRight{ VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT }; // RIGHT
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickDownRight{ VK_PAD_RTHUMB_DOWNRIGHT }; // RIGHT-DOWN
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickDown{ VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN }; // DOWN
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickDownLeft{ VK_PAD_RTHUMB_DOWNLEFT }; // DOWN-LEFT
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickLeft{ VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT }; // LEFT
		static constexpr keyboardtypes::VirtualKey_t RightThumbstickUpLeft{ VK_PAD_RTHUMB_UPLEFT }; // UP-LEFT

		// Other buttons
		static constexpr keyboardtypes::VirtualKey_t ButtonStart{ XINPUT_GAMEPAD_START };
		static constexpr keyboardtypes::VirtualKey_t ButtonBack{ XINPUT_GAMEPAD_BACK };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeft{ XINPUT_GAMEPAD_LEFT_SHOULDER };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderRight{ XINPUT_GAMEPAD_RIGHT_SHOULDER };
		static constexpr keyboardtypes::VirtualKey_t ThumbLeftClick{ XINPUT_GAMEPAD_LEFT_THUMB };
		static constexpr keyboardtypes::VirtualKey_t ThumbRightClick{ XINPUT_GAMEPAD_RIGHT_THUMB };

		// used internally to denote left or right triggers, similar to the button VKs though they may
		// not be used by the OS API state updates in the same way--we virtualize them.
		static constexpr keyboardtypes::VirtualKey_t LeftTrigger{ VK_GAMEPAD_LEFT_TRIGGER };
		static constexpr keyboardtypes::VirtualKey_t RightTrigger{ VK_GAMEPAD_RIGHT_TRIGGER };

		/**
		 * \brief The button virtual keycodes as a flat array.
		 */
		static constexpr std::array<keyboardtypes::VirtualKey_t, 14> ButtonCodeArray
		{
			DpadUp,
			DpadDown,
			DpadLeft,
			DpadRight,
			ButtonStart,
			ButtonBack,
			ThumbLeftClick,
			ThumbRightClick,
			ButtonShoulderLeft,
			ButtonShoulderLeft,
			ButtonA,
			ButtonB,
			ButtonX,
			ButtonY
		};

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
		KeyboardPlayerInfo PlayerInfo;
		KeyboardSettings Settings;
	};
	
	// MagnitudeSentinel is used to trim the adjusted magnitude values from the thumbstick, max on my hardware close to 32k.
	static constexpr int MagnitudeSentinel{ 32'766 };
	// number of coordinate plane quadrants (obviously 4)
	static constexpr auto NumQuadrants = 4;
	static constexpr keyboardtypes::ComputationFloat_t MY_PI{ std::numbers::pi_v<keyboardtypes::ComputationFloat_t> };
	static constexpr keyboardtypes::ComputationFloat_t MY_PI2{ std::numbers::pi_v<keyboardtypes::ComputationFloat_t> / keyboardtypes::ComputationFloat_t{2} };
	static constexpr keyboardtypes::ComputationFloat_t MY_PI8{ std::numbers::pi_v<keyboardtypes::ComputationFloat_t> / keyboardtypes::ComputationFloat_t{8} };
}