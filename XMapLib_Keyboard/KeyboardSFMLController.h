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
		static constexpr VirtualKey_t ButtonUnknown{ 14 };

		using ApiCodePair_t = std::pair<VirtualKey_t, VirtualButtons>;
		static constexpr std::array<ApiCodePair_t, 15> ApiCodeToVirtualButtonArrayPs5
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
			{ButtonShiftSwitch, VirtualButtons::ShiftSwitch},
			{ButtonUnknown, VirtualButtons::RightTrigger}
		} };

		static constexpr std::array<ApiCodePair_t, 17> ApiCodeToVirtualButtonArrayXbox
		{ {
			{ButtonX, VirtualButtons::A},
			{ButtonA, VirtualButtons::B},
			{ButtonB, VirtualButtons::X},
			{ButtonY, VirtualButtons::Y},
			{ButtonShoulderLeft, VirtualButtons::ShoulderLeft},
			{ButtonShoulderRight, VirtualButtons::ShoulderRight},
			{ButtonShoulderLeftLower, VirtualButtons::Back},
			{ButtonShoulderRightLower, VirtualButtons::Start},
			{ButtonLeftPill, VirtualButtons::LeftPill},
			{ButtonRightPill, VirtualButtons::RightPill},
			{ButtonLeftStickClick, VirtualButtons::LeftStickClick},
			{ButtonRightStickClick, VirtualButtons::RightStickClick},
			{ButtonPlayLogo, VirtualButtons::RightTrigger},
			{ButtonShiftSwitch, VirtualButtons::ShiftSwitch},
			{ButtonUnknown, VirtualButtons::RightTrigger},
			{15, VirtualButtons::RightTrigger},
			{16, VirtualButtons::RightTrigger}
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
	 * \brief Translates SFML specific axis/etc. data into the virtual buttons enum values expected for a Sony PS5 controller.
	 * \param playerId Most commonly 0 for a single device connected.
	 * \param stickLeftRightDz Deadzone for all axes of left and right sticks.
	 * \return Vector type of VirtualButtons enum.
	 */
	[[nodiscard]]
	inline
	auto GetWrappedControllerStateUpdatePs5(
		const int playerId = 0,
		const std::pair<float,float> stickLeftRightDz = std::make_pair(30.0f, 30.0f),
		const std::pair<float,float> triggerLeftRightDz = std::make_pair(30.0f, 30.0f)) noexcept -> SmallVector_t<VirtualButtons>
	{
		using Stick = sf::Joystick;
		Stick::update();

		if (Stick::isConnected(playerId))
		{
			// TODO controller trigger gradients instead of built-in on/off.
			//const auto leftTriggerGradient = Stick::getAxisPosition(playerId, Stick::Axis::U); // Ltrigger gradient
			//const auto rightTriggerGradient = Stick::getAxisPosition(playerId, Stick::Axis::V); // Rtrigger gradient
			//constexpr auto leftTriggerIt = std::ranges::find(detail::ApiCodeToVirtualButtonArrayPs5, detail::ButtonShoulderLeftLower, &detail::ApiCodePair_t::first);
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
			for (const auto& [apiCode, virtualCode] : detail::ApiCodeToVirtualButtonArrayPs5)
			{
				if (Stick::isButtonPressed(playerId, apiCode))
					allKeys.push_back(virtualCode);
					
			}

			const auto [leftStickDz, rightStickDz] = stickLeftRightDz;
			if (leftStickRadius > leftStickDz)
				allKeys.push_back(leftStickDirection);
			if (rightStickRadius > rightStickDz)
				allKeys.push_back(rightStickDirection);

			return allKeys;
		}
		return {};
	}

	///**
	// * \brief Translates SFML specific axis/etc. data into the virtual buttons enum values expected for an Xbox (360) controller.
	// * \param playerId Most commonly 0 for a single device connected.
	// * \param stickLeftRightDz Deadzone for all axes of left and right sticks.
	// * \return Vector type of VirtualButtons enum.
	// */
	//[[nodiscard]]
	//inline
	//auto GetWrappedControllerStateUpdateXbox(
	//	const int playerId = 0,
	//	const std::pair<float, float> stickLeftRightDz = std::make_pair(30.0f, 30.0f),
	//	const std::pair<float, float> triggerLeftRightDz = std::make_pair(30.0f, 30.0f)) noexcept -> SmallVector_t<VirtualButtons>
	//{
	//	using Stick = sf::Joystick;
	//	Stick::update();

	//	if (Stick::isConnected(playerId))
	//	{
	//		// Ltrigger and Rtrigger gradient value, -100 is R trigger, +100 is Ltrigger.
	//		const auto triggerGradient = Stick::getAxisPosition(playerId, Stick::Axis::Z);
	//		const auto secondGradient = Stick::getAxisPosition(playerId, Stick::Axis::R);

	//		std::cout << triggerGradient << '\t' << secondGradient << '\n';

	//		const auto thumbstickRightX = Stick::getAxisPosition(playerId, Stick::Axis::U);
	//		const auto thumbstickRightY = Stick::getAxisPosition(playerId, Stick::Axis::V);

	//		const auto thumbstickLeftX = Stick::getAxisPosition(playerId, Stick::Axis::X);
	//		const auto thumbstickLeftY = Stick::getAxisPosition(playerId, Stick::Axis::Y);

	//		const auto [leftStickRadius, leftStickAngle] = ComputePolarPair(thumbstickLeftX, -thumbstickLeftY);
	//		const auto [rightStickRadius, rightStickAngle] = ComputePolarPair(thumbstickRightX, -thumbstickRightY);

	//		const auto leftStickDirection = GetVirtualKeyFromDirection(GetDirectionForPolarTheta(leftStickAngle), ControllerStick::LeftStick);
	//		const auto rightStickDirection = GetVirtualKeyFromDirection(GetDirectionForPolarTheta(rightStickAngle), ControllerStick::RightStick);

	//		SmallVector_t<VirtualButtons> allKeys;
	//		for (const auto& [apiCode, virtualCode] : detail::ApiCodeToVirtualButtonArrayXbox)
	//		{
	//			if (Stick::isButtonPressed(playerId, apiCode))
	//				allKeys.push_back(virtualCode);
	//		}

	//		const auto [leftStickDz, rightStickDz] = stickLeftRightDz;
	//		if (leftStickRadius > leftStickDz)
	//			allKeys.push_back(leftStickDirection);
	//		if (rightStickRadius > rightStickDz)
	//			allKeys.push_back(rightStickDirection);

	//		//const auto [leftTriggerDz, rightTriggerDz] = triggerLeftRightDz;
	//		//const bool isLeftTrigger = triggerGradient > 0;
	//		//const auto triggerDz = isLeftTrigger > 0 ? leftTriggerDz : rightTriggerDz;
	//		//const bool isTriggerPulled = std::abs(triggerGradient) > triggerDz;

	//		//if (triggerGradient > leftTriggerDz)
	//		//	allKeys.push_back(VirtualButtons::LeftTrigger);
	//		//if (rightTriggerGradient > rightTriggerDz)
	//		//	allKeys.push_back(VirtualButtons::RightTrigger);

	//		return allKeys;
	//	}
	//	return {};
	//}
}

