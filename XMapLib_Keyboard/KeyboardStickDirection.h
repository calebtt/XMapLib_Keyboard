#pragma once
#include "KeyboardCustomTypes.h"
#include "KeyboardControllerSettings.h"

#include <numbers>
#include <tuple>
#include <optional>


namespace sds
{
	namespace detail
	{
		// Apparently std::tuple can't be used for non-type template param
		struct DirectionTuple
		{
			keyboardtypes::ComputationFloat_t Low;
			keyboardtypes::ComputationFloat_t High;
			ThumbstickDirection Direction;
			constexpr DirectionTuple(keyboardtypes::ComputationFloat_t low, keyboardtypes::ComputationFloat_t high, ThumbstickDirection dir) noexcept
				: Low(low), High(high), Direction(dir) { }
		};

		using DirectionTuple_t = DirectionTuple;

		// Some values used for polar direction computations
		static constexpr keyboardtypes::ComputationFloat_t MY_PI{ std::numbers::pi_v<keyboardtypes::ComputationFloat_t> };
		static constexpr keyboardtypes::ComputationFloat_t MY_PI2{ std::numbers::pi_v<keyboardtypes::ComputationFloat_t> / keyboardtypes::ComputationFloat_t{2} };
		static constexpr keyboardtypes::ComputationFloat_t MY_PI8{ std::numbers::pi_v<keyboardtypes::ComputationFloat_t> / keyboardtypes::ComputationFloat_t{8} };

		// Direction boundary value tuples, with diagonal directions, and translation.
		static constexpr auto StickRight{ DirectionTuple_t(-MY_PI8, MY_PI8, ThumbstickDirection::Right) };
		static constexpr auto StickUpRight{ DirectionTuple_t(MY_PI8, 3 * MY_PI8, ThumbstickDirection::UpRight) };
		static constexpr auto StickUp{ DirectionTuple_t(3 * MY_PI8, 5 * MY_PI8, ThumbstickDirection::Up) };
		static constexpr auto StickUpLeft{ DirectionTuple_t(5 * MY_PI8, 7 * MY_PI8, ThumbstickDirection::LeftUp) };

		static constexpr auto StickDownRight{ DirectionTuple_t(3 * -MY_PI8, -MY_PI8, ThumbstickDirection::RightDown) };
		static constexpr auto StickDown{ DirectionTuple_t(5 * -MY_PI8, 3 * -MY_PI8, ThumbstickDirection::Down) };
		static constexpr auto StickDownLeft{ DirectionTuple_t(7 * -MY_PI, 5 * -MY_PI8, ThumbstickDirection::DownLeft) };
		static constexpr auto StickLeft{ DirectionTuple_t(7 * MY_PI8, 7 * -MY_PI8, ThumbstickDirection::Left) };

		// Primary function template for a directional bounds checker.
		template<DirectionTuple_t bounds>
		constexpr auto GetDirection(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>
		{
			return (theta >= bounds.Low && theta <= bounds.High) ? bounds.Direction : std::optional<ThumbstickDirection>{};
		}

		// Explicit instantiations for specific boundary pair and direction type.
		template auto GetDirection<StickRight>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;
		template auto GetDirection<StickUpRight>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;
		template auto GetDirection<StickUp>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;
		template auto GetDirection<StickUpLeft>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;

		// This specialization requires custom logic in the bounds check to work.
		template<>
		constexpr auto GetDirection<StickLeft>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>
		{
			const bool isThetaPositive = theta >= decltype(theta){};
			const bool isWithinBounds = isThetaPositive ? theta >= StickLeft.Low : theta <= StickLeft.High;
			return isWithinBounds ? StickLeft.Direction : std::optional<ThumbstickDirection>{};
		}

		template auto GetDirection<StickDownLeft>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;
		template auto GetDirection<StickDown>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;
		template auto GetDirection<StickDownRight>(const keyboardtypes::ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>;
	}


	/**
	 * \brief  Returns ThumbstickDirection enum for the polar theta angle given (values from 0.0 to 3.14 approx)
	 * \param theta  Polar theta angular value (float).
	 */
	[[nodiscard]]
	constexpr
	auto GetDirectionForPolarTheta(const keyboardtypes::ComputationFloat_t theta) noexcept -> ThumbstickDirection
	{
		using namespace detail;
		const auto dir = GetDirection<StickRight>(theta)
			.or_else([theta]() { return GetDirection<StickUpRight>(theta); })
			.or_else([theta]() { return GetDirection<StickUp>(theta); })
			.or_else([theta]() { return GetDirection<StickUpLeft>(theta); })
			.or_else([theta]() { return GetDirection<StickLeft>(theta); })
			.or_else([theta]() { return GetDirection<StickDownLeft>(theta); })
			.or_else([theta]() { return GetDirection<StickDown>(theta); })
			.or_else([theta]() { return GetDirection<StickDownRight>(theta); });
		return dir.value_or(ThumbstickDirection::Invalid);
	}

	/**
	 * \brief  Gets the internal use virtual keycode matching the direction.
	 * \param direction  Direction enum.
	 * \param whichStick  Left or right stick will determine the result.
	 */
	[[nodiscard]]
	constexpr
	auto GetVirtualKeyFromDirection(const ThumbstickDirection direction, const ControllerStick whichStick) -> VirtualButtons
	{
		using namespace detail;
		using enum VirtualButtons;
		const bool isLeftStick = whichStick == ControllerStick::LeftStick;

		switch (direction)
		{
		case ThumbstickDirection::Up: return isLeftStick ? LeftThumbstickUp : RightThumbstickUp;
		case ThumbstickDirection::UpRight: return isLeftStick ? LeftThumbstickUpRight : RightThumbstickUpRight;
		case ThumbstickDirection::Right: return isLeftStick ? LeftThumbstickRight : RightThumbstickRight;
		case ThumbstickDirection::RightDown:  return isLeftStick ? LeftThumbstickDownRight : RightThumbstickDownRight;
		case ThumbstickDirection::Down: return isLeftStick ? LeftThumbstickDown : RightThumbstickDown;
		case ThumbstickDirection::DownLeft: return isLeftStick ? LeftThumbstickDownLeft : RightThumbstickDownLeft;
		case ThumbstickDirection::Left: return isLeftStick ? LeftThumbstickLeft : RightThumbstickLeft;
		case ThumbstickDirection::LeftUp: return isLeftStick ? LeftThumbstickUpLeft : RightThumbstickUpLeft;
		case ThumbstickDirection::Invalid: return NotSet;
		}
		return NotSet;
	}

}
