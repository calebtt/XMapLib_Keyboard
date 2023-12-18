module;

#include <SFML/Main.hpp>
#include <SFML/Graphics.hpp>

export module SfmlController;

import XMapLibBase;
import CustomTypes;
import <array>;
import <tuple>;

namespace detail
{
	// Note, these are the virtual keycodes received from SFML with a ps5 controller.
	constexpr sds::VirtualKey_t ButtonSquare{ 0 };
	constexpr sds::VirtualKey_t ButtonCross{ 1 };
	constexpr sds::VirtualKey_t ButtonCircle{ 2 };
	constexpr sds::VirtualKey_t ButtonTriangle{ 3 };
	constexpr sds::VirtualKey_t ButtonShoulderLeft{ 4 };
	constexpr sds::VirtualKey_t ButtonShoulderRight{ 5 };
	constexpr sds::VirtualKey_t ButtonShoulderLeftLower{ 6 };
	constexpr sds::VirtualKey_t ButtonShoulderRightLower{ 7 };
	constexpr sds::VirtualKey_t ButtonLeftPill{ 8 };
	constexpr sds::VirtualKey_t ButtonRightPill{ 9 };
	constexpr sds::VirtualKey_t ButtonLeftStickClick{ 10 };
	constexpr sds::VirtualKey_t ButtonRightStickClick{ 11 };
	constexpr sds::VirtualKey_t ButtonPlayLogo{ 12 };
	constexpr sds::VirtualKey_t ButtonShiftSwitch{ 13 };
	constexpr sds::VirtualKey_t ButtonUnknown{ 14 };

	// 
	struct ApiCodePair
	{
		sds::VirtualKey_t Key;
		sds::VirtualButtons VirtualButton;
	};

	constexpr ApiCodePair ApiCodeToVirtualButtonArrayPs5[] =
	{
		{ButtonCross, sds::VirtualButtons::X},
		{ButtonSquare, sds::VirtualButtons::Square},
		{ButtonCircle, sds::VirtualButtons::Circle},
		{ButtonTriangle, sds::VirtualButtons::Triangle},
		{ButtonShoulderLeft, sds::VirtualButtons::ShoulderLeft},
		{ButtonShoulderRight, sds::VirtualButtons::ShoulderRight},
		{ButtonShoulderLeftLower, sds::VirtualButtons::LeftTrigger},
		{ButtonShoulderRightLower, sds::VirtualButtons::RightTrigger},
		{ButtonLeftPill, sds::VirtualButtons::LeftPill},
		{ButtonRightPill, sds::VirtualButtons::RightPill},
		{ButtonLeftStickClick, sds::VirtualButtons::LeftStickClick},
		{ButtonRightStickClick, sds::VirtualButtons::RightStickClick},
		{ButtonPlayLogo, sds::VirtualButtons::PS5Logo},
		{ButtonShiftSwitch, sds::VirtualButtons::ShiftSwitch},
		{ButtonUnknown, sds::VirtualButtons::RightTrigger}
	};
}

export namespace sds
{
	[[nodiscard]]
	auto GetWrappedControllerStateUpdatePs5(
		const int playerId,
		const float leftStickDz,
		const float rightStickDz) noexcept -> sds::SmallVector_t<sds::VirtualButtons>
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

			sds::SmallVector_t<sds::VirtualButtons> allKeys;
			for (const auto [apiCode, virtualCode] : ::detail::ApiCodeToVirtualButtonArrayPs5)
			{
				if (Stick::isButtonPressed(playerId, apiCode))
					allKeys.push_back(virtualCode);
			}

			if (leftStickRadius > leftStickDz)
				allKeys.push_back(leftStickDirection);
			if (rightStickRadius > rightStickDz)
				allKeys.push_back(rightStickDirection);
			
			return allKeys;
		}
		return {};
	}
}
