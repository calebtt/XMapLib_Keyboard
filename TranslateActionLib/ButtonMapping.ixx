module;

export module ButtonMapping;

import <string>;
import <optional>;
import <ranges>;
import <memory>;
import XMapLibBase;
import MappingStateTracker;

export namespace sds
{
	enum class RepeatType
	{
		Infinite, // Upon the button being held down, will translate to the key-repeat function activating repeatedly using a delay in between repeats.
		FirstOnly, // Upon the button being held down, will send a single repeat, will not continue translating to repeat after the single repeat.
		None // No key-repeats sent.
	};

	/// <summary>
	///		Functions called when a state change occurs.
	/// </summary>
	struct KeyStateBehaviors final
	{
		Fn_t OnDown; // Key-down
		Fn_t OnUp; // Key-up
		Fn_t OnRepeat; // Key-repeat
		Fn_t OnReset; // Reset after key-up prior to another key-down
	};
	static_assert(std::copyable<KeyStateBehaviors>);
	static_assert(std::movable<KeyStateBehaviors>);

	/**
	 * \brief	Controller button to action mapping. This is how a mapping of a controller button to an action is described.
	 */
	struct ButtonDescription final
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
		 * \brief	The exclusivity grouping member is intended to allow the user to add different groups of mappings
		 *	that require another mapping from the same group to be "overtaken" or key-up sent before the "overtaking" new mapping
		 *	can perform the key-down.
		 * \remarks		optional, if not in use set to default constructed value or '{}'
		 */
		std::optional<GrpVal_t> ExclusivityGrouping; // TODO one variation of ex. group behavior is to have a priority value associated with the mapping.
	public:
		ButtonDescription(
			const VirtualButtons buttonCode,
			std::optional<RepeatType> repeatBehavior = {},
			std::optional<GrpVal_t> optExclusivityGrouping = {}) noexcept
			: ButtonVirtualKeycode(buttonCode),
			RepeatingKeyBehavior(repeatBehavior.value_or(RepeatType::Infinite)),
			ExclusivityGrouping(optExclusivityGrouping)
		{
		}
	};
	static_assert(std::copyable<ButtonDescription>);
	static_assert(std::movable<ButtonDescription>);

	struct MappingContainer final
	{
		/**
		 * \brief	The mapping description.
		 */
		ButtonDescription Button;

		/**
		 * \brief	The events/functions associated with different states.
		 */
		KeyStateBehaviors StateFunctions;

		/**
		 * \brief	Mutable last action performed, with get/set methods.
		*/
		MappingStateTracker LastAction;

	public:
		MappingContainer(
			const ButtonDescription& buttonDescription,
			const KeyStateBehaviors& stateFunctions,
			std::optional<NanosDelay_t> beforeRepeatDelay = {},
			std::optional<NanosDelay_t> betweenRepeatDelay = {})
			:
			Button(buttonDescription),
			StateFunctions(stateFunctions)
		{
			if (beforeRepeatDelay)
				LastAction.DelayBeforeFirstRepeat.Reset(beforeRepeatDelay.value());
			if (betweenRepeatDelay)
				LastAction.LastSentTime.Reset(betweenRepeatDelay.value());
		}

		MappingContainer(
			ButtonDescription&& buttonDescription,
			KeyStateBehaviors&& stateFunctions,
			std::optional<NanosDelay_t> beforeRepeatDelay = {},
			std::optional<NanosDelay_t> betweenRepeatDelay = {})
			:
			Button(std::move(buttonDescription)),
			StateFunctions(std::move(stateFunctions))
		{
			if (beforeRepeatDelay)
				LastAction.DelayBeforeFirstRepeat.Reset(beforeRepeatDelay.value());
			if (betweenRepeatDelay)
				LastAction.LastSentTime.Reset(betweenRepeatDelay.value());
		}
	};
	static_assert(std::copyable<MappingContainer>);
	static_assert(std::movable<MappingContainer>);

	// Concept for range of ButtonDescription type that provides random access.
	template<typename T>
	concept MappingRange_c = requires (T & t)
	{
		{ std::same_as<typename T::value_type, MappingContainer> == true };
		{ std::ranges::random_access_range<T> == true };
	};
}