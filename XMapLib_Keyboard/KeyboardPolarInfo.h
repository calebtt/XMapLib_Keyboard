#pragma once
#include <cassert>
#include <functional>
#include <algorithm>
#include <numeric>
#include <numbers>
#include <array>
#include <ranges>
#include <unordered_map>

#include "KeyboardSettingsPack.h"

namespace sds
{
	[[nodiscard]]
	constexpr
	bool IsFloatZero(const auto testFloat) noexcept
	{
		constexpr auto eps = std::numeric_limits<decltype(testFloat)>::epsilon();
		constexpr auto eps2 = eps * 2;
		return std::abs(testFloat) <= eps2;
	}

	// [FloatingType, FloatingType] wherein the first member is the polar radius, and the second is the polar theta angle.
	using PolarInfoPair = std::pair<keyboardtypes::ComputationFloat_t, keyboardtypes::ComputationFloat_t>;
	/**
	 * \brief Compute polar coordinates pair, [FloatingType, FloatingType] wherein the first member is the polar radius, and the second is the polar theta angle.
	 */
	[[nodiscard]]
	inline
	auto ComputePolarPair(const keyboardtypes::ComputationFloat_t xStickValue, const keyboardtypes::ComputationFloat_t yStickValue) noexcept -> PolarInfoPair
	{
		constexpr auto nonZeroValue{ std::numeric_limits<keyboardtypes::ComputationFloat_t>::min() }; // cannot compute with both values at 0, this is used instead
		const bool areBothZero = IsFloatZero(xStickValue) && IsFloatZero(yStickValue);

		const keyboardtypes::ComputationFloat_t xValue = areBothZero ? nonZeroValue : xStickValue;
		const keyboardtypes::ComputationFloat_t yValue = areBothZero ? nonZeroValue : yStickValue;
		const auto rad = std::hypot(xValue, yValue);
		const auto angle = std::atan2(yValue, xValue);
		return { rad, angle };
	}

}
