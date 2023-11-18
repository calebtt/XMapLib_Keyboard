#pragma once
#include <concepts>
#include "KeyboardCustomTypes.h"
#include "KeyboardVirtualController.h"
#include "DelayTimer.h"
#include "MappingStateTracker.h"

namespace sds
{
	/**
	 * \brief	Controller button to action mapping. This is how a mapping of a controller button to an action is described.
	 */
	struct CBActionMap final
	{
		/**
		 * \brief	Controller button Virtual Keycode. Can be platform dependent or custom mapping, depends on input poller behavior.
		 */
		VirtualButtons ButtonVirtualKeycode{};

		/**
		 * \brief	If 'true', upon the button being held down, will translate to the key-repeat function activating repeatedly
		 *	using a delay in between repeats.
		 */
		bool UsesInfiniteRepeat{ true };

		/**
		 * \brief	If 'true', upon the button being held down, will send a single repeat, will not continue translating to repeat after the single repeat.
		 * \remarks Note that UsesInfiniteRepeat is expected to be set to 'false' for this to have a meaningful impact.
		 */
		bool SendsFirstRepeatOnly{ false };

		/**
		 * \brief	The exclusivity grouping member is intended to allow the user to add different groups of mappings
		 *	that require another mapping from the same group to be "overtaken" or key-up sent before the "overtaking" new mapping
		 *	can perform the key-down.
		 * \remarks		optional, if not in use set to default constructed value or '{}'
		 */
		OptGrp_t ExclusivityGrouping; // TODO one variation of ex. group behavior is to have a priority value associated with the mapping.
		Fn_t OnDown; // Key-down
		Fn_t OnUp; // Key-up
		Fn_t OnRepeat; // Key-repeat
		Fn_t OnReset; // Reset after key-up prior to another key-down
		OptNanosDelay_t DelayBeforeFirstRepeat; // optional custom delay before first key-repeat
		OptNanosDelay_t DelayForRepeats; // optional custom delay between key-repeats
		MappingStateManager LastAction; // Last action performed, with get/set methods.
	public:
		// TODO member funcs for setting delays aren't used because it would screw up the nice braced initialization list mapping construction.
		// TODO maybe should add a real constructor or something? The idea is to encapsulate the data as much as possible.
		// And the OptNanosDelay_t leaves a bit more to be encapsulated.
	};

	static_assert(std::copyable<CBActionMap>);
	static_assert(std::movable<CBActionMap>);

	// Concept for range of CBActionMap type that provides random access.
	template<typename T>
	concept MappingRange_c = requires (T & t)
	{
		{ std::same_as<typename T::value_type, CBActionMap> == true };
		{ std::ranges::random_access_range<T> == true };
	};
}
