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
#include "KeyboardSettingsPack.h"
#include "KeyboardPolarInfo.h"
#include "KeyboardStickDirection.h"

namespace sds::XInput
{
	// detail, locally used items.
	namespace detail
	{
		// Platform specific Controller button codes, these belong here closer to where they are used--
		// They are not configurable, and not relevant to the configuration.
		static constexpr keyboardtypes::VirtualKey_t ButtonA{ XINPUT_GAMEPAD_A };
		static constexpr keyboardtypes::VirtualKey_t ButtonB{ XINPUT_GAMEPAD_B };
		static constexpr keyboardtypes::VirtualKey_t ButtonX{ XINPUT_GAMEPAD_X };
		static constexpr keyboardtypes::VirtualKey_t ButtonY{ XINPUT_GAMEPAD_Y };

		static constexpr keyboardtypes::VirtualKey_t ButtonStart{ XINPUT_GAMEPAD_START };
		static constexpr keyboardtypes::VirtualKey_t ButtonBack{ XINPUT_GAMEPAD_BACK };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeft{ XINPUT_GAMEPAD_LEFT_SHOULDER };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderRight{ XINPUT_GAMEPAD_RIGHT_SHOULDER };
		static constexpr keyboardtypes::VirtualKey_t ThumbLeftClick{ XINPUT_GAMEPAD_LEFT_THUMB };
		static constexpr keyboardtypes::VirtualKey_t ThumbRightClick{ XINPUT_GAMEPAD_RIGHT_THUMB };

		// Dpad
		static constexpr keyboardtypes::VirtualKey_t DpadUp{ XINPUT_GAMEPAD_DPAD_UP };
		static constexpr keyboardtypes::VirtualKey_t DpadDown{ XINPUT_GAMEPAD_DPAD_DOWN };
		static constexpr keyboardtypes::VirtualKey_t DpadLeft{ XINPUT_GAMEPAD_DPAD_LEFT };
		static constexpr keyboardtypes::VirtualKey_t DpadRight{ XINPUT_GAMEPAD_DPAD_RIGHT };

		// Library/mapping use codes, non-platform.
		static constexpr keyboardtypes::VirtualKey_t LeftTrigger{ 201 };
		static constexpr keyboardtypes::VirtualKey_t RightTrigger{ 202 };

		/**
		 * \brief The button virtual keycodes as a flat array.
		 */
		static constexpr std::array<std::pair<keyboardtypes::VirtualKey_t, VirtualButtons>, 14> ApiCodeToVirtualButtonArray
		{ {
			{DpadUp, VirtualButtons::DpadUp},
			{DpadDown, VirtualButtons::DpadDown},
			{DpadLeft, VirtualButtons::DpadLeft},
			{DpadRight, VirtualButtons::DpadRight},
			{ButtonStart, VirtualButtons::Start},
			{ButtonBack, VirtualButtons::Back},
			{ThumbLeftClick, VirtualButtons::LeftStickClick},
			{ThumbRightClick, VirtualButtons::RightStickClick},
			{ButtonShoulderLeft, VirtualButtons::ShoulderLeft},
			{ButtonShoulderRight, VirtualButtons::ShoulderRight},
			{ButtonA, VirtualButtons::A},
			{ButtonB, VirtualButtons::B},
			{ButtonX, VirtualButtons::X},
			{ButtonY, VirtualButtons::Y}
		} };

		/**
		 * \brief The wButtons member of the OS API struct is ONLY for the buttons, triggers are not set there on a key-down.
		 */
		[[nodiscard]] constexpr bool IsLeftTriggerBeyondThreshold(const keyboardtypes::TriggerValue_t triggerValue, const keyboardtypes::TriggerValue_t triggerThreshold) noexcept
		{
			return triggerValue > triggerThreshold;
		}

		[[nodiscard]] constexpr bool IsRightTriggerBeyondThreshold(const keyboardtypes::TriggerValue_t triggerValue, const keyboardtypes::TriggerValue_t triggerThreshold) noexcept
		{
			return triggerValue > triggerThreshold;
		}
	}

	/**
	 * \brief	Important helper function to build a small vector of button VKs that are 'down'. Essential function
	 *	is to decompose bit masked state updates into an array.
	 * \param settingsPack	Settings pertaining to deadzone info and virtual keycodes.
	 * \param controllerState	The OS API state update.
	 * \return	small vector of down buttons.
	 */
	[[nodiscard]]
	inline
	auto GetDownVirtualKeycodesRange(const KeyboardSettingsXInput& settingsPack, const XINPUT_STATE& controllerState) -> keyboardtypes::SmallVector_t<VirtualButtons>
	{
		using namespace detail;
		// Keys
		keyboardtypes::SmallVector_t<VirtualButtons> allKeys{};
		for (const auto& [elem, virtualCode] : ApiCodeToVirtualButtonArray)
		{
			if (controllerState.Gamepad.wButtons & elem)
				allKeys.push_back(virtualCode);
		}

		// Triggers
		if (IsLeftTriggerBeyondThreshold(controllerState.Gamepad.bLeftTrigger, settingsPack.LeftTriggerThreshold))
			allKeys.emplace_back(VirtualButtons::LeftTrigger);
		if (IsRightTriggerBeyondThreshold(controllerState.Gamepad.bRightTrigger, settingsPack.RightTriggerThreshold))
			allKeys.emplace_back(VirtualButtons::RightTrigger);

		// Stick axes
		const auto LeftStickDz{ settingsPack.LeftStickDeadzone };
		const auto RightStickDz{ settingsPack.RightStickDeadzone };

		const auto leftThumbstickX{ controllerState.Gamepad.sThumbLX };
		const auto rightThumbstickX{ controllerState.Gamepad.sThumbRX };

		const auto leftThumbstickY{ controllerState.Gamepad.sThumbLY };
		const auto rightThumbstickY{ controllerState.Gamepad.sThumbRY };

		const auto leftStickPolarInfo{ ComputePolarPair(leftThumbstickX, leftThumbstickY) };
		const auto rightStickPolarInfo{ ComputePolarPair(rightThumbstickX, rightThumbstickY) };

		const auto leftDirection{ GetDirectionForPolarTheta(leftStickPolarInfo.second) };
		const auto rightDirection{ GetDirectionForPolarTheta(rightStickPolarInfo.second) };

		const auto leftThumbstickVk{ GetVirtualKeyFromDirection(leftDirection, ControllerStick::LeftStick) };
		const auto rightThumbstickVk{ GetVirtualKeyFromDirection(rightDirection, ControllerStick::RightStick) };

		const bool leftIsDown = leftStickPolarInfo.first > LeftStickDz && leftThumbstickVk.has_value();
		const bool rightIsDown = rightStickPolarInfo.first > RightStickDz && rightThumbstickVk.has_value();

		if (leftIsDown)
			allKeys.emplace_back(leftThumbstickVk.value());
		if (rightIsDown)
			allKeys.emplace_back(rightThumbstickVk.value());

		return allKeys;
	}

	/**
	 * \brief Calls the OS API function(s).
	 * \param playerId Most commonly 0 for a single device connected.
	 * \return Platform/API specific state structure.
	 */
	[[nodiscard]]
	inline
	auto GetLegacyApiStateUpdate(const int playerId = 0) noexcept -> XINPUT_STATE
	{
		XINPUT_STATE controllerState{};
		XInputGetState(playerId, &controllerState);
		return controllerState;
	}

	/**
	 * \brief Gets a wrapped controller state update.
	 * \param settingsPack Settings information, not necessarily constexpr or compile time.
	 * \return Wrapper for the controller state buffer.
	 */
	[[nodiscard]]
	inline
	auto GetWrappedLegacyApiStateUpdate(const KeyboardSettingsPack& settingsPack) noexcept -> keyboardtypes::SmallVector_t<VirtualButtons>
	{
		return GetDownVirtualKeycodesRange(settingsPack.Settings, GetLegacyApiStateUpdate(settingsPack.PlayerInfo.PlayerId));
	}
}
