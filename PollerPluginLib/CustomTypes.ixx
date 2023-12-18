export module CustomTypes;

import <vector>;
import <functional>;
import <optional>;
import <tuple>;
import <chrono>;
import <map>;
import <cstdint>;

export namespace sds
{
	constexpr std::size_t SmallBufferSize{ 32 };
	using Fn_t = std::function<void()>;
	using OptFn_t = std::optional<Fn_t>;
	using NanosDelay_t = std::chrono::nanoseconds;
	using OptNanosDelay_t = std::optional<NanosDelay_t>;
	using GrpVal_t = std::uint32_t;
	using OptGrp_t = std::optional<GrpVal_t>;
	using VirtualKey_t = std::int32_t;
	using TriggerValue_t = std::uint8_t; // Hardware trigger value
	using ThumbstickValue_t = std::int16_t; // Hardware thumbstick value
	using Index_t = std::uint32_t; // Nothing we work with will have more elements than a 32 bit uint can position for.

	// Some types and values for use with cartesian/polar computations, that kind of thing.
	using ComputationFloat_t = float; // float type for computations
	using CompFloatPair_t = std::pair<ComputationFloat_t, ComputationFloat_t>; // Computation float pair, usually bounds of a range
	using ComputationStickValue_t = int; // Controller stick value type for computations

	template<typename T>
	using SmallVector_t = std::vector<T>;

	template<typename T, typename V>
	using SmallFlatMap_t = std::map<T, V>;
}