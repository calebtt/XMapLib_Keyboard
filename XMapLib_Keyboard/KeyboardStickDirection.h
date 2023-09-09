#pragma once
#include <numbers>
#include <tuple>
#include "KeyboardSettingsPack.h"

namespace sds
{
	namespace detail
	{
		// Direction boundary value pairs, with diagonals.
		static constexpr auto StickRight{ std::make_pair(-MY_PI8, MY_PI8) };
		static constexpr auto StickUpRight{ std::make_pair(MY_PI8, 3 * MY_PI8) };
		static constexpr auto StickUp{ std::make_pair(3 * MY_PI8, 5 * MY_PI8) };
		static constexpr auto StickUpLeft{ std::make_pair(5 * MY_PI8, 7 * MY_PI8) };

		static constexpr auto StickDownRight{ std::make_pair(3 * -MY_PI8, -MY_PI8) };
		static constexpr auto StickDown{ std::make_pair(5 * -MY_PI8, 3 * -MY_PI8) };
		static constexpr auto StickDownLeft{ std::make_pair(7 * -MY_PI, 5 * -MY_PI8) };
		static constexpr auto StickLeft{ std::make_pair(7 * MY_PI8, 7 * -MY_PI8) };

		// Primary template for a directional bounds checker.
		template<keyboardtypes::CompFloatPair_t bounds, ThumbstickDirection direction>
		struct IsDirection
		{
			static constexpr ThumbstickDirection Direction{ direction };

			static
			constexpr
			bool IsItThisDirection(const keyboardtypes::ComputationFloat_t theta) noexcept
			{
				return theta >= bounds.first && theta <= bounds.second;
			}
		};

		// Explicit instantiations for specific boundary pair and direction type.
		template struct IsDirection<StickRight, ThumbstickDirection::Right>;
		template struct IsDirection<StickUpRight, ThumbstickDirection::UpRight>;
		template struct IsDirection<StickUp, ThumbstickDirection::Up>;
		template struct IsDirection<StickUpLeft, ThumbstickDirection::LeftUp>;
		// This specialization requires custom logic in the IsItThisDirection() to work.
		template<> struct IsDirection<StickLeft, ThumbstickDirection::Left>
		{
			static constexpr ThumbstickDirection Direction{ ThumbstickDirection::Left };

			static
			constexpr
			bool IsItThisDirection(const keyboardtypes::ComputationFloat_t theta) noexcept
			{
				const bool isThetaPositive = theta >= decltype(theta){};
				return isThetaPositive ? theta >= StickLeft.first : theta <= StickLeft.second;
			}
		};
		template struct IsDirection<StickDownLeft, ThumbstickDirection::DownLeft>;
		template struct IsDirection<StickDown, ThumbstickDirection::Down>;
		template struct IsDirection<StickDownRight, ThumbstickDirection::RightDown>;

		// Type aliases for the instantiations/specializations provided.
		using DirectionFuncRight = IsDirection<StickRight, ThumbstickDirection::Right>;
		using DirectionFuncUpRight = IsDirection<StickUpRight, ThumbstickDirection::UpRight>;
		using DirectionFuncRightUp = DirectionFuncUpRight;
		using DirectionFuncUp = IsDirection<StickUp, ThumbstickDirection::Up>;
		using DirectionFuncUpLeft = IsDirection<StickUpLeft, ThumbstickDirection::LeftUp>;
		using DirectionFuncLeftUp = DirectionFuncUpLeft;
		using DirectionFuncLeft = IsDirection<StickLeft, ThumbstickDirection::Left>;
		using DirectionFuncDownLeft = IsDirection<StickDownLeft, ThumbstickDirection::DownLeft>;
		using DirectionFuncLeftDown = DirectionFuncDownLeft;
		using DirectionFuncDown = IsDirection<StickDown, ThumbstickDirection::Down>;
		using DirectionFuncDownRight = IsDirection<StickDownRight, ThumbstickDirection::RightDown>;
		using DirectionFuncRightDown = DirectionFuncDownRight;

	}

	[[nodiscard]]
	constexpr
	auto GetDirectionForPolarTheta(const keyboardtypes::ComputationFloat_t theta) noexcept -> ThumbstickDirection
	{
		using namespace detail;

		if (DirectionFuncRight::IsItThisDirection(theta))
			return DirectionFuncRight::Direction;
		if (DirectionFuncRightUp::IsItThisDirection(theta))
			return DirectionFuncRightUp::Direction;
		if (DirectionFuncUp::IsItThisDirection(theta))
			return DirectionFuncUp::Direction;
		if (DirectionFuncUpLeft::IsItThisDirection(theta))
			return DirectionFuncUpLeft::Direction;
		if (DirectionFuncLeft::IsItThisDirection(theta))
			return DirectionFuncLeft::Direction;
		if (DirectionFuncLeftDown::IsItThisDirection(theta))
			return DirectionFuncLeftDown::Direction;
		if (DirectionFuncDown::IsItThisDirection(theta))
			return DirectionFuncDown::Direction;
		if (DirectionFuncDownRight::IsItThisDirection(theta))
			return DirectionFuncDownRight::Direction;

		return ThumbstickDirection::Invalid;
	}

	[[nodiscard]]
	constexpr
	auto GetVirtualKeyFromDirection(const KeyboardSettings& settingsPack, const ThumbstickDirection direction, const ControllerStick whichStick) -> std::optional<keyboardtypes::VirtualKey_t>
	{
		const bool isLeftStick = whichStick == ControllerStick::LeftStick;

		switch (direction)
		{
		case ThumbstickDirection::Up: return isLeftStick ? settingsPack.LeftThumbstickUp : settingsPack.RightThumbstickUp;
		case ThumbstickDirection::UpRight: return isLeftStick ? settingsPack.LeftThumbstickUpRight : settingsPack.RightThumbstickUpRight;
		case ThumbstickDirection::Right: return isLeftStick ? settingsPack.LeftThumbstickRight : settingsPack.RightThumbstickRight;
		case ThumbstickDirection::RightDown:  return isLeftStick ? settingsPack.LeftThumbstickDownRight : settingsPack.RightThumbstickDownRight;
		case ThumbstickDirection::Down: return isLeftStick ? settingsPack.LeftThumbstickDown : settingsPack.RightThumbstickDown;
		case ThumbstickDirection::DownLeft: return isLeftStick ? settingsPack.LeftThumbstickDownLeft : settingsPack.RightThumbstickDownLeft;
		case ThumbstickDirection::Left: return isLeftStick ? settingsPack.LeftThumbstickLeft : settingsPack.RightThumbstickLeft;
		case ThumbstickDirection::LeftUp: return isLeftStick ? settingsPack.LeftThumbstickUpLeft : settingsPack.RightThumbstickUpLeft;
		case ThumbstickDirection::Invalid: return {};
		default:
		{
			throw std::runtime_error("Bad mapping of ThumbstickDirection to virtual key.");
		}
		}
		return {};
	}

}
