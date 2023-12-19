module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")

export module PollerXbox;

import XMapLibBase;
import <vector>;
import <ranges>;
import <algorithm>;
import <array>;
import <utility>;
import <cmath>;
import <tuple>;
import <optional>;

export namespace sds
{
	// detail, locally used items.
	namespace detail
	{
		// Platform specific Controller button codes, these belong here closer to where they are used--
		// They are not configurable, and not relevant to the configuration.
		constexpr sds::VirtualKey_t ButtonA{ XINPUT_GAMEPAD_A };
		constexpr sds::VirtualKey_t ButtonB{ XINPUT_GAMEPAD_B };
		constexpr sds::VirtualKey_t ButtonX{ XINPUT_GAMEPAD_X };
		constexpr sds::VirtualKey_t ButtonY{ XINPUT_GAMEPAD_Y };

		constexpr sds::VirtualKey_t ButtonStart{ XINPUT_GAMEPAD_START };
		constexpr sds::VirtualKey_t ButtonBack{ XINPUT_GAMEPAD_BACK };
		constexpr sds::VirtualKey_t ButtonShoulderLeft{ XINPUT_GAMEPAD_LEFT_SHOULDER };
		constexpr sds::VirtualKey_t ButtonShoulderRight{ XINPUT_GAMEPAD_RIGHT_SHOULDER };
		constexpr sds::VirtualKey_t ThumbLeftClick{ XINPUT_GAMEPAD_LEFT_THUMB };
		constexpr sds::VirtualKey_t ThumbRightClick{ XINPUT_GAMEPAD_RIGHT_THUMB };

		// Dpad
		constexpr sds::VirtualKey_t DpadUp{ XINPUT_GAMEPAD_DPAD_UP };
		constexpr sds::VirtualKey_t DpadDown{ XINPUT_GAMEPAD_DPAD_DOWN };
		constexpr sds::VirtualKey_t DpadLeft{ XINPUT_GAMEPAD_DPAD_LEFT };
		constexpr sds::VirtualKey_t DpadRight{ XINPUT_GAMEPAD_DPAD_RIGHT };

		// Library/mapping use codes, non-platform.
		constexpr sds::VirtualKey_t LeftTrigger{ 201 };
		constexpr sds::VirtualKey_t RightTrigger{ 202 };

		constexpr sds::ThumbstickValue_t LeftStickDeadzone{ XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE };
		constexpr sds::ThumbstickValue_t RightStickDeadzone{ XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE };

		constexpr sds::TriggerValue_t LeftTriggerThreshold{ XINPUT_GAMEPAD_TRIGGER_THRESHOLD };
		constexpr sds::TriggerValue_t RightTriggerThreshold{ XINPUT_GAMEPAD_TRIGGER_THRESHOLD };

		struct ApiCodePair
		{
			sds::VirtualKey_t Key;
			sds::VirtualButtons VirtualButton;
		};

		/**
		 * \brief The button virtual keycodes as a flat array.
		 */
		constexpr ApiCodePair ApiCodeToVirtualButtonArray[] =
		{
			{DpadUp, sds::VirtualButtons::DpadUp},
			{DpadDown, sds::VirtualButtons::DpadDown},
			{DpadLeft, sds::VirtualButtons::DpadLeft},
			{DpadRight, sds::VirtualButtons::DpadRight},
			{ButtonStart, sds::VirtualButtons::Start},
			{ButtonBack, sds::VirtualButtons::Back},
			{ThumbLeftClick, sds::VirtualButtons::LeftStickClick},
			{ThumbRightClick, sds::VirtualButtons::RightStickClick},
			{ButtonShoulderLeft, sds::VirtualButtons::ShoulderLeft},
			{ButtonShoulderRight, sds::VirtualButtons::ShoulderRight},
			{ButtonA, sds::VirtualButtons::A},
			{ButtonB, sds::VirtualButtons::B},
			{ButtonX, sds::VirtualButtons::X},
			{ButtonY, sds::VirtualButtons::Y}
		};

		/**
		 * \brief The wButtons member of the OS API struct is ONLY for the buttons, triggers are not set there on a key-down.
		 */
		[[nodiscard]] constexpr bool IsLeftTriggerBeyondThreshold(const sds::TriggerValue_t triggerValue, const sds::TriggerValue_t triggerThreshold) noexcept
		{
			return triggerValue > triggerThreshold;
		}

		[[nodiscard]] constexpr bool IsRightTriggerBeyondThreshold(const sds::TriggerValue_t triggerValue, const sds::TriggerValue_t triggerThreshold) noexcept
		{
			return triggerValue > triggerThreshold;
		}
	}

	/**
	 * \brief	Important helper function to build a small vector of button VKs that are 'down'. Essential function is to decompose bit masked 
			state updates into an array.
	 * \param controllerState	The OS API state update.
	 * \return	small vector of down buttons.
	 */
	[[nodiscard]]
	inline auto GetDownVirtualKeycodesRange(
		const XINPUT_STATE& controllerState,
		const int playerId,
		const float leftStickDz,
		const float rightStickDz) -> SmallVector_t<VirtualButtons>
	{
		using namespace detail;

		// Keys
		SmallVector_t<VirtualButtons> allKeys{};
		for (const auto [key, value] : ApiCodeToVirtualButtonArray)
		{
			if (controllerState.Gamepad.wButtons & key)
				allKeys.push_back(value);
		}

		// Triggers
		if (IsLeftTriggerBeyondThreshold(controllerState.Gamepad.bLeftTrigger, LeftTriggerThreshold))
			allKeys.emplace_back(LeftTrigger);
		if (IsRightTriggerBeyondThreshold(controllerState.Gamepad.bRightTrigger, RightTriggerThreshold))
			allKeys.emplace_back(RightTrigger);

		// Stick axes
		constexpr auto LeftStickDz{ LeftStickDeadzone };
		constexpr auto RightStickDz{ RightStickDeadzone };

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

		// TODO deadzone appears too low on left stick, check this part.
		const bool leftIsDown = leftStickPolarInfo.first > LeftStickDz;
		const bool rightIsDown = rightStickPolarInfo.first > RightStickDz;

		if (leftIsDown)
			allKeys.emplace_back(leftThumbstickVk);
		if (rightIsDown)
			allKeys.emplace_back(rightThumbstickVk);

		return allKeys;
	}

	/**
	 * \brief Calls the OS API function(s).
	 * \param playerId Most commonly 0 for a single device connected.
	 * \return Platform/API specific state structure.
	 */
	[[nodiscard]]
	inline auto GetLegacyApiStateUpdate(const int playerId = 0) noexcept -> XINPUT_STATE
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
	inline auto GetWrappedLegacyApiStateUpdate(const int playerId) noexcept -> SmallVector_t<VirtualButtons>
	{
		return GetDownVirtualKeycodesRange(GetLegacyApiStateUpdate(playerId), playerId, detail::LeftStickDeadzone, detail::RightStickDeadzone);
	}
}