#pragma once
#include <array>
#include "KeyboardCustomTypes.h"
#include "KeyboardVirtualController.h"
#include "KeyboardPolarInfo.h"
#include "KeyboardStickDirection.h"
#include <SFML/window/Joystick.hpp>

namespace sds
{
	namespace detail
	{
		// Note, these are the virtual keycodes received from SFML with a ps5 controller.
		static constexpr VirtualKey_t ButtonX{ 0 };
		static constexpr VirtualKey_t ButtonA{ 1 };
		static constexpr VirtualKey_t ButtonB{ 2 };
		static constexpr VirtualKey_t ButtonY{ 3 };
		static constexpr VirtualKey_t ButtonShoulderLeft{ 4 };
		static constexpr VirtualKey_t ButtonShoulderRight{ 5 };
		static constexpr VirtualKey_t ButtonShoulderLeftLower{ 6 };
		static constexpr VirtualKey_t ButtonShoulderRightLower{ 7 };
		static constexpr VirtualKey_t ButtonLeftPill{ 8 };
		static constexpr VirtualKey_t ButtonRightPill{ 9 };
		static constexpr VirtualKey_t ButtonLeftStickClick{ 10 };
		static constexpr VirtualKey_t ButtonRightStickClick{ 11 };
		static constexpr VirtualKey_t ButtonPlayLogo{ 12 };
		static constexpr VirtualKey_t ButtonShiftSwitch{ 13 };

		using ApiCodePair_t = std::pair<VirtualKey_t, VirtualButtons>;
		static constexpr std::array<ApiCodePair_t, 14> ApiCodeToVirtualButtonArray
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

	// Returns controller connected status.
	[[nodiscard]]
	inline
	bool IsControllerConnected(const int pid) noexcept
	{
		return sf::Joystick::isConnected(pid);
	}

	/**
	 * \brief Uses SFML to call some OS API function(s) to retrieve a controller state update.
	 * \param leftStickDz Deadzone for all axes of left stick.
	 * \param rightStickDz Deadzone for all axes of right stick.
	 * \param playerId Most commonly 0 for a single device connected.
	 * \return Platform/API specific state structure.
	 */
	[[nodiscard]]
	inline
	auto GetWrappedControllerStateUpdate(const int playerId = 0, 
		const float leftStickDz = 30.0f, 
		const float rightStickDz = 30.0f) noexcept -> SmallVector_t<VirtualButtons>
	{
		using Stick = sf::Joystick;
		Stick::update();

		if (Stick::isConnected(playerId))
		{
			// TODO controller trigger gradients instead of built-in on/off.
			//const auto leftTriggerGradient = Stick::getAxisPosition(playerId, Stick::Axis::U); // Ltrigger gradient
			//const auto rightTriggerGradient = Stick::getAxisPosition(playerId, Stick::Axis::V); // Rtrigger gradient
			//constexpr auto leftTriggerIt = std::ranges::find(detail::ApiCodeToVirtualButtonArray, detail::ButtonShoulderLeftLower, &detail::ApiCodePair_t::first);
			//constexpr auto leftTriggerVirtual = leftTriggerIt->second;

			const auto thumbstickRightX = Stick::getAxisPosition(playerId, Stick::Axis::Z);
			const auto thumbstickRightY = Stick::getAxisPosition(playerId, Stick::Axis::R);

			const auto thumbstickLeftX = Stick::getAxisPosition(playerId, Stick::Axis::X);
			const auto thumbstickLeftY = Stick::getAxisPosition(playerId, Stick::Axis::Y);

			const auto [leftStickRadius, leftStickAngle] = ComputePolarPair(thumbstickLeftX, -thumbstickLeftY);
			const auto [rightStickRadius, rightStickAngle] = ComputePolarPair(thumbstickRightX, -thumbstickRightY);

			const auto leftStickDirection = GetVirtualKeyFromDirection(GetDirectionForPolarTheta(leftStickAngle), ControllerStick::LeftStick);
			const auto rightStickDirection = GetVirtualKeyFromDirection(GetDirectionForPolarTheta(rightStickAngle), ControllerStick::RightStick);

			SmallVector_t<VirtualButtons> allKeys;
			for (const auto& [apiCode, virtualCode] : detail::ApiCodeToVirtualButtonArray)
			{
				if (Stick::isButtonPressed(playerId, apiCode))
					allKeys.push_back(virtualCode);
			}
			if (leftStickRadius >= leftStickDz)
				allKeys.push_back(leftStickDirection);
			if (rightStickRadius >= rightStickDz)
				allKeys.push_back(rightStickDirection);

			return allKeys;
		}
		return {};
	}
}

