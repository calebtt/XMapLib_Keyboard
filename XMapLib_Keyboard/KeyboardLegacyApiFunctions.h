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

namespace sds
{
	// TODO for thumbstick and trigger magnitudes.
	//struct DownKeyInfo
	//{
	//	keyboardtypes::VirtualKey_t VirtualCode;
	//	int Magnitude;
	//};

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

	/**
	 * \brief	Important helper function to build a small vector of button VKs that are 'down'. Essential function
	 *	is to decompose bit masked state updates into an array.
	 * \param settingsPack	Settings pertaining to deadzone info and virtual keycodes.
	 * \param controllerState	The OS API state update.
	 * \return	small vector of down buttons.
	 */
	[[nodiscard]]
	inline
	auto GetDownVirtualKeycodesRange(const KeyboardSettings& settingsPack, const XINPUT_STATE& controllerState) -> keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>
	{
		keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t> allKeys{};
		for (const auto elem : settingsPack.ButtonCodeArray)
		{
			if (controllerState.Gamepad.wButtons & elem)
			{
				allKeys.emplace_back(elem);
			}
		}

		if (IsLeftTriggerBeyondThreshold(controllerState.Gamepad.bLeftTrigger, settingsPack.LeftTriggerThreshold))
			allKeys.emplace_back(settingsPack.LeftTriggerVk);
		if (IsRightTriggerBeyondThreshold(controllerState.Gamepad.bRightTrigger, settingsPack.RightTriggerThreshold))
			allKeys.emplace_back(settingsPack.RightTriggerVk);

		constexpr auto LeftStickDz{ settingsPack.LeftStickDeadzone };
		constexpr auto RightStickDz{ settingsPack.RightStickDeadzone };
		if (controllerState.Gamepad.sThumbLX < (-LeftStickDz))
			allKeys.emplace_back(settingsPack.LeftThumbstickLeft);
		if (controllerState.Gamepad.sThumbLX > LeftStickDz)
			allKeys.emplace_back(settingsPack.LeftThumbstickRight);
		if (controllerState.Gamepad.sThumbLY > LeftStickDz)
			allKeys.emplace_back(settingsPack.LeftThumbstickUp);
		if (controllerState.Gamepad.sThumbLY < (-LeftStickDz))
			allKeys.emplace_back(settingsPack.LeftThumbstickDown);

		if (controllerState.Gamepad.sThumbRX < (-RightStickDz))
			allKeys.emplace_back(settingsPack.RightThumbstickLeft);
		if (controllerState.Gamepad.sThumbRX > RightStickDz)
			allKeys.emplace_back(settingsPack.RightThumbstickRight);
		if (controllerState.Gamepad.sThumbRY > RightStickDz)
			allKeys.emplace_back(settingsPack.RightThumbstickUp);
		if (controllerState.Gamepad.sThumbRY < (-RightStickDz))
			allKeys.emplace_back(settingsPack.RightThumbstickDown);
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
	auto GetWrappedLegacyApiStateUpdate(const KeyboardSettingsPack& settingsPack) noexcept -> keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>
	{
		return GetDownVirtualKeycodesRange(settingsPack.Settings, GetLegacyApiStateUpdate(settingsPack.PlayerInfo.PlayerId));
	}
}