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
	namespace detail
	{
		//array of boundary values, used to determine which polar quadrant a polar angle resides in
		static constexpr std::array<const std::pair<keyboardtypes::ComputationFloat_t, keyboardtypes::ComputationFloat_t>, NumQuadrants> QuadrantBoundsArray
		{
			std::make_pair(keyboardtypes::ComputationFloat_t{}, MY_PI2),
			std::make_pair(MY_PI2, MY_PI),
			std::make_pair(-MY_PI, -MY_PI2),
			std::make_pair(-MY_PI2, keyboardtypes::ComputationFloat_t{})
		};

	}

	[[nodiscard]]
	constexpr
	bool IsFloatZero(const auto testFloat) noexcept
	{
		constexpr auto eps = std::numeric_limits<decltype(testFloat)>::epsilon();
		constexpr auto eps2 = eps * 2;
		return std::abs(testFloat) <= eps2;
	}

	// Pair[Pair[double,double], int] wherein the inner pair is the quadrant bounds, and the outer int is the quadrant number. 
	using QuadrantInfoPair = std::pair<keyboardtypes::CompFloatPair_t, int>;
	/**
	 * \brief Retrieves begin and end range values for the quadrant the polar theta (angle) value resides in, and the quadrant number (0-indexed)
	 * \return Pair[Pair[double,double], int] wherein the inner pair is the quadrant bounds, and the outer int is the quadrant number. 
	 */
	[[nodiscard]]
	constexpr
	auto GetQuadrantInfo(const keyboardtypes::ComputationFloat_t polarTheta) -> QuadrantInfoPair
	{
		using std::ranges::views::enumerate;
		using std::get;

		constexpr auto indexedBounds = detail::QuadrantBoundsArray | enumerate;
		for(const auto [index, bounds] : indexedBounds)
		{
			const auto lowerBound = get<0>(bounds);
			const auto upperBound = get<1>(bounds);
			if (polarTheta >= lowerBound && polarTheta <= upperBound)
				return { { bounds }, static_cast<int>(index) };
		}
		throw std::exception{};
	}

	//[int, int] wherein the first member is the adjusted X magnitude value, and the second is the adjusted Y magnitude
	using AdjustedMagnitudePair = std::pair<int, int>;

	//trim computed magnitude values to sentinel value
	[[nodiscard]]
	constexpr
	auto TrimMagnitudeToSentinel(const int x, const int y) noexcept -> AdjustedMagnitudePair
	{
		auto tempX = x;
		auto tempY = y;
		tempX = std::clamp(tempX, -MagnitudeSentinel, MagnitudeSentinel);
		tempY = std::clamp(tempY, -MagnitudeSentinel, MagnitudeSentinel);
		return { tempX, tempY };
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

	//compute adjusted magnitudes
	[[nodiscard]]
	constexpr
	auto ComputeAdjustedMagnitudes(const PolarInfoPair& polarInfo, const QuadrantInfoPair& quadInfo) noexcept -> AdjustedMagnitudePair
	{
		const auto& [polarRadius, polarTheta] = polarInfo;
		const auto& [quadrantSentinelPair, quadrantNumber] = quadInfo;
		const auto& [quadrantBeginVal, quadrantSentinelVal] = quadrantSentinelPair;
		//compute proportion of the radius for each axis to be the axial magnitudes, apparently a per-quadrant calculation with my setup.
		const auto redPortion = (polarTheta - quadrantBeginVal) * polarRadius;
		const auto blackPortion = (quadrantSentinelVal - polarTheta) * polarRadius;
		const double xProportion = quadrantNumber % 2 ? blackPortion : redPortion;
		const double yProportion = quadrantNumber % 2 ? redPortion : blackPortion;
		return TrimMagnitudeToSentinel(static_cast<keyboardtypes::ComputationStickValue_t>(xProportion), static_cast<keyboardtypes::ComputationStickValue_t>(yProportion));
	}

	using PolarCompleteInfoTuple = std::tuple<PolarInfoPair, QuadrantInfoPair, AdjustedMagnitudePair>;
	[[nodiscard]]
	inline
	auto ComputePolarCompleteInfo(const keyboardtypes::ComputationStickValue_t xStickValue, const keyboardtypes::ComputationStickValue_t yStickValue) noexcept -> PolarCompleteInfoTuple
	{
		using keyboardtypes::ComputationFloat_t;

		PolarCompleteInfoTuple tempPack;
		auto& polarPair = std::get<0>(tempPack);
		auto& quadrantInfo = std::get<1>(tempPack);
		auto& adjustedMagnitudes = std::get<2>(tempPack);

		polarPair = ComputePolarPair(static_cast<ComputationFloat_t>(xStickValue), static_cast<ComputationFloat_t>(yStickValue));
		quadrantInfo = GetQuadrantInfo(std::get<1>(polarPair));
		adjustedMagnitudes = ComputeAdjustedMagnitudes(polarPair, quadrantInfo);
		return tempPack;
	}
}
