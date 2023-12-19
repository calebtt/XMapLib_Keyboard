export module StickDirection;
import CustomTypes;
import VirtualController;
import <chrono>;
import <numeric>;
import <numbers>;
import <optional>;

export namespace sds
{
	namespace detail
	{
		// Apparently std::tuple can't be used for non-type template param
		struct DirectionTuple
		{
			sds::ComputationFloat_t Low;
			sds::ComputationFloat_t High;
			ThumbstickDirection Direction;
			constexpr DirectionTuple(ComputationFloat_t low, ComputationFloat_t high, ThumbstickDirection dir) noexcept
				: Low(low), High(high), Direction(dir) { }
		};

		using DirectionTuple_t = DirectionTuple;

		// Some values used for polar direction computations
		constexpr sds::ComputationFloat_t MY_PI{ std::numbers::pi_v<ComputationFloat_t> };
		constexpr sds::ComputationFloat_t MY_PI2{ std::numbers::pi_v<ComputationFloat_t> / ComputationFloat_t{2} };
		constexpr sds::ComputationFloat_t MY_PI8{ std::numbers::pi_v<ComputationFloat_t> / ComputationFloat_t{8} };

		// Direction boundary value tuples, with diagonal directions, and translation.
		constexpr auto StickRight{ DirectionTuple_t(-MY_PI8, MY_PI8, ThumbstickDirection::Right) };
		constexpr auto StickUpRight{ DirectionTuple_t(MY_PI8, 3 * MY_PI8, ThumbstickDirection::UpRight) };
		constexpr auto StickUp{ DirectionTuple_t(3 * MY_PI8, 5 * MY_PI8, ThumbstickDirection::Up) };
		constexpr auto StickUpLeft{ DirectionTuple_t(5 * MY_PI8, 7 * MY_PI8, ThumbstickDirection::LeftUp) };

		constexpr auto StickDownRight{ DirectionTuple_t(3 * -MY_PI8, -MY_PI8, ThumbstickDirection::RightDown) };
		constexpr auto StickDown{ DirectionTuple_t(5 * -MY_PI8, 3 * -MY_PI8, ThumbstickDirection::Down) };
		constexpr auto StickDownLeft{ DirectionTuple_t(7 * -MY_PI, 5 * -MY_PI8, ThumbstickDirection::DownLeft) };
		constexpr auto StickLeft{ DirectionTuple_t(7 * MY_PI8, 7 * -MY_PI8, ThumbstickDirection::Left) };

		// Primary function template for a directional bounds checker.
		template<DirectionTuple_t bounds>
		constexpr auto GetDirection(const ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>
		{
			return (theta >= bounds.Low && theta <= bounds.High) ? bounds.Direction : std::optional<ThumbstickDirection>{};
		}

		// This specialization requires custom logic in the bounds check to work.
		template<>
		constexpr auto GetDirection<StickLeft>(const ComputationFloat_t theta) noexcept -> std::optional<ThumbstickDirection>
		{
			const bool isThetaPositive = theta >= decltype(theta){};
			const bool isWithinBounds = isThetaPositive ? theta >= StickLeft.Low : theta <= StickLeft.High;
			return isWithinBounds ? StickLeft.Direction : std::optional<ThumbstickDirection>{};
		}
	}

	/**
	 * \brief  Returns ThumbstickDirection enum for the polar theta angle given (values from 0.0 to 3.14 approx)
	 * \param theta  Polar theta angular value (float).
	 */
	[[nodiscard]]
	constexpr auto GetDirectionForPolarTheta(const ComputationFloat_t theta) noexcept -> ThumbstickDirection
	{
		using namespace detail;
		// Higher order fn that returns a fn
		auto RaiseFnWithTheta = [theta](auto&& fnToCall)
			{
				return [theta, &fnToCall]() { return fnToCall(theta); };
			};
		const auto dir = GetDirection<StickRight>(theta)
			.or_else(RaiseFnWithTheta(GetDirection<StickUpRight>))
			.or_else(RaiseFnWithTheta(GetDirection<StickUp>))
			.or_else(RaiseFnWithTheta(GetDirection<StickUpLeft>))
			.or_else(RaiseFnWithTheta(GetDirection<StickLeft>))
			.or_else(RaiseFnWithTheta(GetDirection<StickDownLeft>))
			.or_else(RaiseFnWithTheta(GetDirection<StickDown>))
			.or_else(RaiseFnWithTheta(GetDirection<StickDownRight>));
		return dir.value_or(ThumbstickDirection::Invalid);
	}

	/**
	 * \brief  Gets the internal use virtual keycode matching the direction.
	 * \param direction  Direction enum.
	 * \param whichStick  Left or right stick will determine the result.
	 */
	[[nodiscard]]
	constexpr auto GetVirtualKeyFromDirection(const ThumbstickDirection direction, const ControllerStick whichStick) -> VirtualButtons
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