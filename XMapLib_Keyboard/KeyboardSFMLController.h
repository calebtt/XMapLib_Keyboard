#pragma once
#include <SFML/window/Joystick.hpp>
#include "KeyboardPolarInfo.h"

#include <string>
#include <iostream>
#include <limits>

namespace sds::PS5
{
	namespace detail
	{
		// Note, these are the virtual keycodes received from SFML with a ps5 controller.
		static constexpr keyboardtypes::VirtualKey_t ButtonX{ 0 };
		static constexpr keyboardtypes::VirtualKey_t ButtonA{ 1 };
		static constexpr keyboardtypes::VirtualKey_t ButtonB{ 2 };
		static constexpr keyboardtypes::VirtualKey_t ButtonY{ 3 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeft{ 4 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderRight{ 5 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeftLower{ 6 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderRightLower{ 7 };
		static constexpr keyboardtypes::VirtualKey_t ButtonLeftPill{ 8 };
		static constexpr keyboardtypes::VirtualKey_t ButtonRightPill{ 9 };
		static constexpr keyboardtypes::VirtualKey_t ButtonLeftStickClick{ 10 };
		static constexpr keyboardtypes::VirtualKey_t ButtonRightStickClick{ 11 };
		static constexpr keyboardtypes::VirtualKey_t ButtonPlayLogo{ 12 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShiftSwitch{ 13 };

		static constexpr std::array<std::pair<keyboardtypes::VirtualKey_t, VirtualButtons>, 14> ApiCodeToVirtualButtonArray
		{ {
			{ButtonX, VirtualButtons::X},
			{ButtonA, VirtualButtons::A},
			{ButtonB, VirtualButtons::B},
			{ButtonY, VirtualButtons::Y},
			{ButtonShoulderLeft, VirtualButtons::ShoulderLeft},
			{ButtonShoulderRight, VirtualButtons::ShoulderRight},
			{ButtonShoulderLeftLower, VirtualButtons::LeftTrigger},
			{ButtonShoulderRightLower, VirtualButtons::RightTrigger},
			{ButtonLeftPill, VirtualButtons::LeftPill},
			{ButtonRightPill, VirtualButtons::RightPill},
			{ButtonLeftStickClick, VirtualButtons::LeftStickClick},
			{ButtonRightStickClick, VirtualButtons::RightStickClick},
			{ButtonPlayLogo, VirtualButtons::PS5Logo},
			{ButtonShiftSwitch, VirtualButtons::ShiftSwitch}
		} };
	}

	struct KeyboardSettingsSFMLPlayStation5
	{
		keyboardtypes::ThumbstickValue_t LeftStickDeadzone{ 30 };
		keyboardtypes::ThumbstickValue_t RightStickDeadzone{ 30 };
	};

	/**
	 * \brief Uses SFML to call some OS API function(s) to retrieve a controller state update.
	 * \param settings A poller specific settings struct with things like deadzone info.
	 * \param playerId Most commonly 0 for a single device connected.
	 * \return Platform/API specific state structure.
	 */
	[[nodiscard]]
	inline
	auto GetWrappedControllerStateUpdate(const KeyboardSettingsSFMLPlayStation5& settings, const int playerId = 0) noexcept -> keyboardtypes::SmallVector_t<VirtualButtons>
	{
		using Stick = sf::Joystick;
		const auto leftStickDeadzone{ settings.LeftStickDeadzone };
		const auto rightStickDeadzone{ settings.RightStickDeadzone };

		Stick::update();

		if (Stick::isConnected(playerId))
		{
			keyboardtypes::SmallVector_t<VirtualButtons> allKeys;

			for (const auto& [apiCode, virtualCode] : detail::ApiCodeToVirtualButtonArray)
			{
				if (Stick::isButtonPressed(playerId, apiCode))
					allKeys.emplace_back(virtualCode);
			}
			// TODO add stick directions here
			//const auto uPos = Stick::getAxisPosition(playerId, Stick::Axis::U); // Ltrigger gradient
			//const auto vPos = Stick::getAxisPosition(playerId, Stick::Axis::V); // Rtrigger gradient

			const auto thumbstickRightX = Stick::getAxisPosition(playerId, Stick::Axis::Z);
			const auto thumbstickRightY = Stick::getAxisPosition(playerId, Stick::Axis::R);

			const auto thumbstickLeftX = Stick::getAxisPosition(playerId, Stick::Axis::X);
			const auto thumbstickLeftY = Stick::getAxisPosition(playerId, Stick::Axis::Y);

			const auto [leftStickRadius, leftStickAngle] = ComputePolarPair(thumbstickLeftX, -thumbstickLeftY);
			const auto [rightStickRadius, rightStickAngle] = ComputePolarPair(thumbstickRightX, -thumbstickRightY);

			const auto leftStickDirection = GetVirtualKeyFromDirection(GetDirectionForPolarTheta(leftStickAngle), ControllerStick::LeftStick);
			const auto rightStickDirection = GetVirtualKeyFromDirection(GetDirectionForPolarTheta(rightStickAngle), ControllerStick::RightStick);

			if (leftStickRadius >= leftStickDeadzone)
				allKeys.emplace_back(leftStickDirection);
			if (rightStickRadius >= rightStickDeadzone)
				allKeys.emplace_back(rightStickDirection);

			return allKeys;
		}
		return {};
	}
}

