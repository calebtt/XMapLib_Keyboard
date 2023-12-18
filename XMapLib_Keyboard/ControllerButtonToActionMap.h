#pragma once
#include <concepts>
#include "KeyboardCustomTypes.h"
#include "KeyboardVirtualController.h"
#include "DelayTimer.h"
#include "MappingStateTracker.h"

namespace sds
{
	/**
	 * \brief	Base class (of evil) for doing external polymorphism.
	 */
	struct KeyStateBehaviorsConcept
	{
		virtual void OnDown() = 0;
		virtual void OnUp() = 0;
		virtual void OnRepeat() = 0;
		virtual void OnReset() = 0;
		virtual ~KeyStateBehaviorsConcept() = default;
	};
	
	/**
	 * \brief	Non-polymorphic type, instead of std::function these can all be templated on a lambda or something.
	 * \tparam DownFn 
	 * \tparam UpFn 
	 * \tparam RepeatFn 
	 * \tparam ResetFn 
	 */
	template
	<
		typename DownFn = Fn_t,
		typename UpFn = Fn_t,
		typename RepeatFn = Fn_t,
		typename ResetFn = Fn_t
	>
	struct KeyStateBehaviors final
	{
		DownFn OnDown; // Key-down
		UpFn OnUp; // Key-up
		RepeatFn OnRepeat; // Key-repeat
		ResetFn OnReset; // Reset after key-up prior to another key-down
	};

	template<typename StateFunctionsPack = KeyStateBehaviors<>>
	class KeyStateBehaviorsImpl final : KeyStateBehaviorsConcept
	{
	private:
		StateFunctionsPack m_stateChangeFunctions;
	public:
		KeyStateBehaviorsImpl(StateFunctionsPack&& keyBehaviorsPack) : m_stateChangeFunctions(std::move(keyBehaviorsPack))
		{ }
		void OnDown() override
		{
			m_stateChangeFunctions.OnDown();
		}
		void OnUp() override
		{
			m_stateChangeFunctions.OnUp();
		}
		void OnRepeat() override
		{
			m_stateChangeFunctions.OnRepeat();
		}
		void OnReset() override
		{
			m_stateChangeFunctions.OnReset();
		}
	};

	enum class RepeatType
	{
		Infinite, // Upon the button being held down, will translate to the key-repeat function activating repeatedly using a delay in between repeats.
		FirstOnly, // Upon the button being held down, will send a single repeat, will not continue translating to repeat after the single repeat.
		None // No key-repeats sent.
	};

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
		 * \brief	Type of key-repeat behavior.
		 */
		RepeatType RepeatingKeyBehavior{};

		/**
		 * \brief	The events/functions associated with different states.
		 */
		std::shared_ptr<KeyStateBehaviorsConcept> StateFunctions;

		/**
		 * \brief	The exclusivity grouping member is intended to allow the user to add different groups of mappings
		 *	that require another mapping from the same group to be "overtaken" or key-up sent before the "overtaking" new mapping
		 *	can perform the key-down.
		 * \remarks		optional, if not in use set to default constructed value or '{}'
		 */
		std::optional<GrpVal_t> ExclusivityGrouping; // TODO one variation of ex. group behavior is to have a priority value associated with the mapping.

		MappingStateManager LastAction; // Mutable last action performed, with get/set methods.
	public:
		CBActionMap(
			const VirtualButtons buttonCode, 
			const RepeatType repeatBehavior,
			std::shared_ptr<KeyStateBehaviorsConcept> behaviorFns,
			std::optional<GrpVal_t> optExclusivityGrouping = {},
			std::optional<NanosDelay_t> beforeRepeatDelay = {}, 
			std::optional<NanosDelay_t> betweenRepeatDelay = {}) :
		ButtonVirtualKeycode(buttonCode),
		RepeatingKeyBehavior(repeatBehavior),
		StateFunctions(std::move(behaviorFns)),
		ExclusivityGrouping(optExclusivityGrouping)
		{
			if (beforeRepeatDelay)
				LastAction.DelayBeforeFirstRepeat.Reset(beforeRepeatDelay.value());
			if (betweenRepeatDelay)
				LastAction.LastSentTime.Reset(betweenRepeatDelay.value());
		}
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
