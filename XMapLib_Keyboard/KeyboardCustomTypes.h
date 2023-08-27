#pragma once

#ifndef USERHAS_BOOST
#define USERHAS_BOOST
#endif

#include <deque>
#include <functional>
#include <optional>
#include <chrono>
#include <vector>
//#include <flat_map>
#include <map>

#ifdef USERHAS_BOOST
#include <boost/container/small_vector.hpp>
#include <boost/container/flat_map.hpp>
#endif

namespace sds::keyboardtypes
{
	static constexpr std::size_t SmallBufferSize{ 32 };
	using Fn_t = std::function<void()>;
	using OptFn_t = std::optional<Fn_t>;
	using NanosDelay_t = std::chrono::nanoseconds;
	using OptNanosDelay_t = std::optional<NanosDelay_t>;
	using GrpVal_t = std::uint32_t;
	using OptGrp_t = std::optional<GrpVal_t>;
	using VirtualKey_t = int32_t;
	using TriggerValue_t = uint8_t;
	using ThumbstickValue_t = int16_t;
	using Index_t = uint32_t; // Nothing we work with will have more elements than a 32 bit uint can position for.

	template<typename T>
	using Deque_t = std::deque<T>;

#ifdef USERHAS_BOOST
	template<typename From, typename To>
	using SmallFlatMap_t = boost::container::small_flat_map<From, To, SmallBufferSize>;

	template<typename T>
	using SmallVector_t = boost::container::small_vector<T, SmallBufferSize>;
#else
	template<typename From, typename To>
	using SmallFlatMap_t = std::map<From, To>;

	template<typename T>
	using SmallVector_t = std::vector<T>;
#endif

}