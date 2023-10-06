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
	enum class ControllerStick : int32_t
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

	/**
	 * \brief	All known/supported controller button/functionality identifiers usable with one or more pollers.
	 */
	enum class VirtualButtons : keyboardtypes::VirtualKey_t
	{
		NotSet, // Invalid value.

		X,
		A,
		B,
		Y,

		LeftTrigger, // Xbox trigger
		RightTrigger, // Xbox trigger

		ShoulderLeft, // top button on PS5
		ShoulderRight, // top button on PS5

		ShoulderBottomLeft, // PS5
		ShoulderBottomRight, // PS5
		LeftPill, // PS5
		RightPill, // PS5
		PS5Logo, // PS5
		ShiftSwitch, // PS5

		LeftStickClick,
		RightStickClick,

		Start,
		Back,

		DpadUp,
		DpadDown,
		DpadLeft,
		DpadRight,

		LeftThumbstickUp,
		LeftThumbstickUpRight,
		LeftThumbstickRight,
		LeftThumbstickDownRight,
		LeftThumbstickDown,
		LeftThumbstickDownLeft,
		LeftThumbstickLeft,
		LeftThumbstickUpLeft,

		RightThumbstickUp,
		RightThumbstickUpRight,
		RightThumbstickRight,
		RightThumbstickDownRight,
		RightThumbstickDown,
		RightThumbstickDownLeft,
		RightThumbstickLeft,
		RightThumbstickUpLeft
	};

	/**
	 * \brief A data structure to hold player information. A default constructed
	 * KeyboardPlayerInfo struct has default values that are usable. 
	 */
	struct KeyboardPlayerInfo final
	{
		int PlayerId{ 0 };
	};

	/**
	 * \brief Some constants that are configurable.
	 */
	struct KeyboardSettingsXInput final
	{
		keyboardtypes::ThumbstickValue_t LeftStickDeadzone{XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE};
		keyboardtypes::ThumbstickValue_t RightStickDeadzone{XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE};

		keyboardtypes::TriggerValue_t LeftTriggerThreshold{XINPUT_GAMEPAD_TRIGGER_THRESHOLD};
		keyboardtypes::TriggerValue_t RightTriggerThreshold{XINPUT_GAMEPAD_TRIGGER_THRESHOLD};
	};

	static_assert(std::copyable<KeyboardSettingsXInput>);

	/**
	 * \brief For no other reason but to make the common task of injecting these down the architecture less verbose.
	 */
	struct KeyboardSettingsPack final
	{
		KeyboardPlayerInfo PlayerInfo;
		KeyboardSettingsXInput Settings;
	};
}