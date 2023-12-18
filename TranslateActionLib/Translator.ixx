module;

export module Translator;

import <string>;
import <stdexcept>;
import <concepts>;
import <ranges>;
import <type_traits>;
import <optional>;


import CustomTypes;
import VirtualController;
import TranslationResult;
import ButtonMapping;
/*
 *	Note: There are some static sized arrays used here with capacity defined in customtypes.
 */

export namespace sds
{
	// A translator type, wherein you can call GetUpdatedState with a range of virtual keycode integral values,
	// and get a TranslationPack as a result.
	template<typename Poller_t>
	concept InputTranslator_c = requires(Poller_t & t)
	{
		{ t.GetUpdatedState({ VirtualButtons::A, VirtualButtons::X, VirtualButtons::Y }) } -> std::convertible_to<TranslationPack>;
	};

	/*
	 *	NOTE: Testing these functions may be quite easy, pass a single ButtonDescription in a certain state to all of these functions,
	 *	and if more than one TranslationResult is produced (aside from perhaps the reset translation), then it would obviously be in error.
	 */

	 /**
	  * \brief For a single mapping, search the controller state update buffer and produce a TranslationResult appropriate to the current mapping state and controller state.
	  * \param downKeys Wrapper class containing the results of a controller state update polling.
	  * \param singleButton The mapping type for a single virtual key of the controller.
	  * \returns Optional, <c>TranslationResult</c>
	  */
	template<typename Val_t>
	[[nodiscard]]
	auto GetButtonTranslationForInitialToDown(const SmallVector_t<Val_t>& downKeys, MappingContainer& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using
			std::ranges::find,
			std::ranges::end;

		if (singleButton.LastAction.IsInitialState())
		{
			const auto findResult = find(downKeys, singleButton.Button.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the down translation.
			if (findResult != end(downKeys))
				return GetInitialKeyDownTranslationResult(singleButton);
		}
		return {};
	}

	template<typename Val_t>
	[[nodiscard]]
	auto GetButtonTranslationForDownToRepeat(const SmallVector_t<Val_t>& downKeys, MappingContainer& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		const bool isDownAndUsesRepeat = singleButton.LastAction.IsDown() &&
			(singleButton.Button.RepeatingKeyBehavior == RepeatType::Infinite || singleButton.Button.RepeatingKeyBehavior == RepeatType::FirstOnly);
		const bool isDelayElapsed = singleButton.LastAction.DelayBeforeFirstRepeat.IsElapsed();
		if (isDownAndUsesRepeat && isDelayElapsed)
		{
			const auto findResult = find(downKeys, singleButton.Button.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the repeat translation.
			if (findResult != end(downKeys))
				return GetRepeatTranslationResult(singleButton);
		}
		return {};
	}

	template<typename Val_t>
	[[nodiscard]]
	auto GetButtonTranslationForRepeatToRepeat(const SmallVector_t<Val_t>& downKeys, MappingContainer& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		const bool isRepeatAndUsesInfinite = singleButton.LastAction.IsRepeating() && singleButton.Button.RepeatingKeyBehavior == RepeatType::Infinite;
		if (isRepeatAndUsesInfinite && singleButton.LastAction.LastSentTime.IsElapsed())
		{
			const auto findResult = find(downKeys, singleButton.Button.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the repeat translation.
			if (findResult != end(downKeys))
				return GetRepeatTranslationResult(singleButton);
		}
		return {};
	}

	template<typename Val_t>
	[[nodiscard]]
	auto GetButtonTranslationForDownOrRepeatToUp(const SmallVector_t<Val_t>& downKeys, MappingContainer& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		if (singleButton.LastAction.IsDown() || singleButton.LastAction.IsRepeating())
		{
			const auto findResult = find(downKeys, singleButton.Button.ButtonVirtualKeycode);
			// If VK is not found in the down list, create the up translation.
			if (findResult == end(downKeys))
				return GetKeyUpTranslationResult(singleButton);
		}
		return {};
	}

	// This is the reset translation
	[[nodiscard]]
	inline auto GetButtonTranslationForUpToInitial(MappingContainer& singleButton) noexcept -> std::optional<TranslationResult>
	{
		// if the timer has elapsed, update back to the initial state.
		if (singleButton.LastAction.IsUp() && singleButton.LastAction.LastSentTime.IsElapsed())
		{
			return GetResetTranslationResult(singleButton);
		}
		return {};
	}

	/**
	 * \brief Encapsulates the mapping buffer, processes controller state updates, returns translation packs.
	 * \remarks If, before destruction, the mappings are in a state other than initial or awaiting reset, then you may wish to
	 *	make use of the <c>GetCleanupActions()</c> function. Not copyable. Is movable.
	 *	<p></p>
	 *	<p>An invariant exists such that: <b>There must be only one mapping per virtual keycode.</b></p>
	 */
	class KeyboardTranslator final
	{
		using MappingVector_t = SmallVector_t<MappingContainer>;
		static_assert(MappingRange_c<MappingVector_t>);

		MappingVector_t m_mappings;

	public:
		KeyboardTranslator() = delete; // no default
		KeyboardTranslator(const KeyboardTranslator& other) = delete; // no copy
		auto operator=(const KeyboardTranslator& other)->KeyboardTranslator & = delete; // no copy-assign

		KeyboardTranslator(KeyboardTranslator&& other) = default; // move-construct
		auto operator=(KeyboardTranslator&& other)->KeyboardTranslator & = default; // move-assign
		~KeyboardTranslator() = default;

		/**
		 * \brief Mapping Vector move Ctor, may throw on exclusivity group error, OR more than one mapping per VK.
		 * \param keyMappings Rv ref to a mapping vector type.
		 * \exception std::runtime_error on exclusivity group error during construction, OR more than one mapping per VK.
		 */
		explicit KeyboardTranslator(MappingVector_t&& keyMappings)
			: m_mappings(std::move(keyMappings))
		{
			if (!AreMappingsUniquePerVk(m_mappings) || !AreMappingVksNonZero(m_mappings))
				throw std::runtime_error("Exception: More than 1 mapping per VK!");
		}
	public:
		[[nodiscard]]
		auto operator()(SmallVector_t<VirtualButtons>&& stateUpdate) noexcept -> TranslationPack
		{
			return GetUpdatedState(std::move(stateUpdate));
		}

		[[nodiscard]]
		auto GetUpdatedState(SmallVector_t<VirtualButtons>&& stateUpdate) noexcept -> TranslationPack
		{
			TranslationPack translations;
			for (auto& mapping : m_mappings)
			{
				if (const auto upToInitial = GetButtonTranslationForUpToInitial(mapping))
				{
					translations.UpdateRequests.push_back(*upToInitial);
				}
				else if (const auto initialToDown = GetButtonTranslationForInitialToDown(stateUpdate, mapping))
				{
					// Advance to next state.
					translations.DownRequests.push_back(*initialToDown);
				}
				else if (const auto downToFirstRepeat = GetButtonTranslationForDownToRepeat(stateUpdate, mapping))
				{
					translations.RepeatRequests.push_back(*downToFirstRepeat);
				}
				else if (const auto repeatToRepeat = GetButtonTranslationForRepeatToRepeat(stateUpdate, mapping))
				{
					translations.RepeatRequests.push_back(*repeatToRepeat);
				}
				else if (const auto repeatToUp = GetButtonTranslationForDownOrRepeatToUp(stateUpdate, mapping))
				{
					translations.UpRequests.push_back(*repeatToUp);
				}
			}
			return translations;
		}

		[[nodiscard]]
		auto GetCleanupActions() noexcept -> SmallVector_t<TranslationResult>
		{
			SmallVector_t<TranslationResult> translations;
			for (auto& mapping : m_mappings)
			{
				if (DoesMappingNeedCleanup(mapping.LastAction))
				{
					translations.push_back(GetKeyUpTranslationResult(mapping));
				}
			}
			return translations;
		}
	};

	static_assert(InputTranslator_c<KeyboardTranslator>);
	static_assert(std::movable<KeyboardTranslator>);
	static_assert(std::copyable<KeyboardTranslator> == false);

}