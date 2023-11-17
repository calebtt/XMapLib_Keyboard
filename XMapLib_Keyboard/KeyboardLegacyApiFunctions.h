#pragma once
//#ifndef NOMINMAX
//#define NOMINMAX
//#endif
//#ifndef WIN32_LEAN_AND_MEAN
//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
//#endif
//#include <Windows.h>
//#include <Xinput.h>
//
//#include "KeyboardCustomTypes.h"
//#include "KeyboardPolarInfo.h"
//#include "KeyboardStickDirection.h"
//
//namespace sds
//{
//	// detail, locally used items.
//	namespace detail
//	{
//		// Platform specific Controller button codes, these belong here closer to where they are used--
//		// They are not configurable, and not relevant to the configuration.
//		static constexpr VirtualKey_t ButtonA{ XINPUT_GAMEPAD_A };
//		static constexpr VirtualKey_t ButtonB{ XINPUT_GAMEPAD_B };
//		static constexpr VirtualKey_t ButtonX{ XINPUT_GAMEPAD_X };
//		static constexpr VirtualKey_t ButtonY{ XINPUT_GAMEPAD_Y };
//
//		static constexpr VirtualKey_t ButtonStart{ XINPUT_GAMEPAD_START };
//		static constexpr VirtualKey_t ButtonBack{ XINPUT_GAMEPAD_BACK };
//		static constexpr VirtualKey_t ButtonShoulderLeft{ XINPUT_GAMEPAD_LEFT_SHOULDER };
//		static constexpr VirtualKey_t ButtonShoulderRight{ XINPUT_GAMEPAD_RIGHT_SHOULDER };
//		static constexpr VirtualKey_t ThumbLeftClick{ XINPUT_GAMEPAD_LEFT_THUMB };
//		static constexpr VirtualKey_t ThumbRightClick{ XINPUT_GAMEPAD_RIGHT_THUMB };
//
//		// Dpad
//		static constexpr VirtualKey_t DpadUp{ XINPUT_GAMEPAD_DPAD_UP };
//		static constexpr VirtualKey_t DpadDown{ XINPUT_GAMEPAD_DPAD_DOWN };
//		static constexpr VirtualKey_t DpadLeft{ XINPUT_GAMEPAD_DPAD_LEFT };
//		static constexpr VirtualKey_t DpadRight{ XINPUT_GAMEPAD_DPAD_RIGHT };
//
//		// Library/mapping use codes, non-platform.
//		static constexpr VirtualKey_t LeftTrigger{ 201 };
//		static constexpr VirtualKey_t RightTrigger{ 202 };
//
//		/**
//		 * \brief The button virtual keycodes as a flat array.
//		 */
//		static constexpr std::array<std::pair<VirtualKey_t, VirtualButtons>, 14> ApiCodeToVirtualButtonArray
//		{ {
//			{DpadUp, VirtualButtons::DpadUp},
//			{DpadDown, VirtualButtons::DpadDown},
//			{DpadLeft, VirtualButtons::DpadLeft},
//			{DpadRight, VirtualButtons::DpadRight},
//			{ButtonStart, VirtualButtons::Start},
//			{ButtonBack, VirtualButtons::Back},
//			{ThumbLeftClick, VirtualButtons::LeftStickClick},
//			{ThumbRightClick, VirtualButtons::RightStickClick},
//			{ButtonShoulderLeft, VirtualButtons::ShoulderLeft},
//			{ButtonShoulderRight, VirtualButtons::ShoulderRight},
//			{ButtonA, VirtualButtons::A},
//			{ButtonB, VirtualButtons::B},
//			{ButtonX, VirtualButtons::X},
//			{ButtonY, VirtualButtons::Y}
//		} };
//
//		/**
//		 * \brief The wButtons member of the OS API struct is ONLY for the buttons, triggers are not set there on a key-down.
//		 */
//		[[nodiscard]] constexpr bool IsLeftTriggerBeyondThreshold(const TriggerValue_t triggerValue, const TriggerValue_t triggerThreshold) noexcept
//		{
//			return triggerValue > triggerThreshold;
//		}
//
//		[[nodiscard]] constexpr bool IsRightTriggerBeyondThreshold(const TriggerValue_t triggerValue, const TriggerValue_t triggerThreshold) noexcept
//		{
//			return triggerValue > triggerThreshold;
//		}
//	}
//
//	/**
//	 * \brief Some settings values that are configurable.
//	 * \remarks A person will wish to construct one of these for use with a poller function.
//	 */
//	struct KeyboardSettingsXInput
//	{
//		ThumbstickValue_t LeftStickDeadzone{ XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE };
//		ThumbstickValue_t RightStickDeadzone{ XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE };
//
//		TriggerValue_t LeftTriggerThreshold{ XINPUT_GAMEPAD_TRIGGER_THRESHOLD };
//		TriggerValue_t RightTriggerThreshold{ XINPUT_GAMEPAD_TRIGGER_THRESHOLD };
//	};
//	static_assert(std::copyable<KeyboardSettingsXInput>);
//
//	/**
//	 * \brief	Important helper function to build a small vector of button VKs that are 'down'. Essential function
//	 *	is to decompose bit masked state updates into an array.
//	 * \param controllerState	The OS API state update.
//	 * \return	small vector of down buttons.
//	 */
//	[[nodiscard]]
//	inline
//	auto GetDownVirtualKeycodesRange(const XINPUT_STATE& controllerState, 
//		const ThumbstickValue_t leftStickDz = 7849,
//		const ThumbstickValue_t rightStickDz = 8689,
//		const TriggerValue_t leftTriggerLimit = 30,
//		const TriggerValue_t rightTriggerLimit = 30) -> SmallVector_t<VirtualButtons>
//	{
//		using namespace detail;
//		// Keys
//		SmallVector_t<VirtualButtons> allKeys{};
//		for (const auto& [elem, virtualCode] : ApiCodeToVirtualButtonArray)
//		{
//			if (controllerState.Gamepad.wButtons & elem)
//				allKeys.push_back(virtualCode);
//		}
//
//		// Triggers
//		if (IsLeftTriggerBeyondThreshold(controllerState.Gamepad.bLeftTrigger, leftTriggerLimit))
//			allKeys.push_back(VirtualButtons::LeftTrigger);
//		if (IsRightTriggerBeyondThreshold(controllerState.Gamepad.bRightTrigger, rightTriggerLimit))
//			allKeys.push_back(VirtualButtons::RightTrigger);
//
//		// Stick axes
//		const auto LeftStickDz{ leftStickDz };
//		const auto RightStickDz{ rightStickDz };
//
//		const auto leftThumbstickX{ controllerState.Gamepad.sThumbLX };
//		const auto rightThumbstickX{ controllerState.Gamepad.sThumbRX };
//
//		const auto leftThumbstickY{ controllerState.Gamepad.sThumbLY };
//		const auto rightThumbstickY{ controllerState.Gamepad.sThumbRY };
//
//		const auto leftStickPolarInfo{ ComputePolarPair(leftThumbstickX, leftThumbstickY) };
//		const auto rightStickPolarInfo{ ComputePolarPair(rightThumbstickX, rightThumbstickY) };
//
//		const auto leftDirection{ GetDirectionForPolarTheta(leftStickPolarInfo.second) };
//		const auto rightDirection{ GetDirectionForPolarTheta(rightStickPolarInfo.second) };
//
//		const bool leftIsDown = leftStickPolarInfo.first > static_cast<ComputationFloat_t>(LeftStickDz);
//		const bool rightIsDown = rightStickPolarInfo.first > static_cast<ComputationFloat_t>(RightStickDz);
//
//		if (leftIsDown)
//			allKeys.push_back(GetVirtualKeyFromDirection(leftDirection, ControllerStick::LeftStick));
//		if (rightIsDown)
//			allKeys.push_back(GetVirtualKeyFromDirection(rightDirection, ControllerStick::RightStick));
//
//		return allKeys;
//	}
//
//	/**
//	 * \brief Calls the OS API function(s).
//	 * \param playerId Most commonly 0 for a single device connected.
//	 * \return Platform/API specific state structure.
//	 */
//	[[nodiscard]]
//	inline
//	auto GetLegacyApiStateUpdate(const int playerId = 0) noexcept -> XINPUT_STATE
//	{
//		XINPUT_STATE controllerState{};
//		XInputGetState(playerId, &controllerState);
//		return controllerState;
//	}
//
//	/**
//	 * \brief  Function of note, returns a wrapped controller state update that can be passed to a translator/filter.
//	 * \param settings Settings information, deadzone and trigger thresholds, etc.
//	 * \param playerId The player number in the case that multiple controllers are connected.
//	 * \return Wrapper for the controller state buffer.
//	 */
//	[[nodiscard]]
//	inline
//	auto GetWrappedControllerStateUpdate(const int playerId = 0, 
//		const ThumbstickValue_t leftStickDz = 7849,
//		const ThumbstickValue_t rightStickDz = 8689,
//		const TriggerValue_t leftTriggerLimit = 30,
//		const TriggerValue_t rightTriggerLimit = 30) noexcept -> SmallVector_t<VirtualButtons>
//	{
//		return GetDownVirtualKeycodesRange(GetLegacyApiStateUpdate(playerId), leftStickDz, rightStickDz, leftTriggerLimit, rightTriggerLimit);
//	}
//}
