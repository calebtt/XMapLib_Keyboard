#pragma once
#include <stdexcept>
#include <concepts>
#include <ranges>
#include <type_traits>

#include "KeyboardCustomTypes.h"
#include "KeyboardTranslationHelpers.h"
#include "KeyboardOvertakingFilter.h"

/*
 *	Note: There are some static sized arrays used here with capacity defined in customtypes.
 */

namespace sds
{
	template<typename Poller_t>
	concept InputPoller_c = requires(Poller_t & t)
	{
		{ t.GetUpdatedState({ 1, 2, 3 }) } -> std::convertible_to<TranslationPack>;
	};

	// Concept for range of CBActionMap type that provides random access.
	template<typename T>
	concept MappingRange_c = requires (T & t)
	{
		{ std::same_as<typename T::value_type, CBActionMap> == true };
		{ std::ranges::random_access_range<T> == true };
	};

	// Concept for a filter class, used to apply a specific "overtaking" behavior (exclusivity grouping behavior) implementation.
	template<typename FilterType_t>
	concept ValidFilterType = requires(FilterType_t & t)
	{
		{ t.SetMappingRange(std::span<CBActionMap>{}) };
		{ t.GetFilteredButtonState({1,2,3}) } -> std::convertible_to<keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>>;
		{ std::movable<FilterType_t> == true };
	};

	/*
	 *	NOTE: Testing these functions may be quite easy, pass a single CBActionMap in a certain state to all of these functions,
	 *	and if more than one TranslationResult is produced (aside from perhaps the reset translation), then it would obviously be in error.
	 */

	/**
	 * \brief For a single mapping, search the controller state update buffer and produce a TranslationResult appropriate to the current mapping state and controller state.
	 * \param downKeys Wrapper class containing the results of a controller state update polling.
	 * \param singleButton The mapping type for a single virtual key of the controller.
	 * \returns Optional, <c>TranslationResult</c>
	 */
	[[nodiscard]]
	inline
	auto GetButtonTranslationForInitialToDown(const keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>& downKeys, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using
		std::ranges::find,
		std::ranges::end;

		if (singleButton.LastAction.IsInitialState())
		{
			const auto findResult = find(downKeys, singleButton.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the down translation.
			if(findResult != end(downKeys))
				return GetInitialKeyDownTranslationResult(singleButton);
		}
		return {};
	}

	[[nodiscard]]
	inline
	auto GetButtonTranslationForDownToRepeat(const keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>& downKeys, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		const bool isDownAndUsesRepeat = singleButton.LastAction.IsDown() && (singleButton.UsesInfiniteRepeat || singleButton.SendsFirstRepeatOnly);
		const bool isDelayElapsed = singleButton.LastAction.DelayBeforeFirstRepeat.IsElapsed();
		if (isDownAndUsesRepeat && isDelayElapsed)
		{
			const auto findResult = find(downKeys, singleButton.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the repeat translation.
			if (findResult != end(downKeys))
				return GetRepeatTranslationResult(singleButton);
		}
		return {};
	}

	[[nodiscard]]
	inline
	auto GetButtonTranslationForRepeatToRepeat(const keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>& downKeys, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		const bool isRepeatAndUsesInfinite = singleButton.LastAction.IsRepeating() && singleButton.UsesInfiniteRepeat;
		if (isRepeatAndUsesInfinite && singleButton.LastAction.LastSentTime.IsElapsed())
		{
			const auto findResult = find(downKeys, singleButton.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the repeat translation.
			if (findResult != end(downKeys))
				return GetRepeatTranslationResult(singleButton);
		}
		return {};
	}

	[[nodiscard]]
	inline
	auto GetButtonTranslationForDownOrRepeatToUp(const keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>& downKeys, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		if (singleButton.LastAction.IsDown() || singleButton.LastAction.IsRepeating())
		{
			const auto findResult = find(downKeys, singleButton.ButtonVirtualKeycode);
			// If VK is not found in the down list, create the up translation.
			if(findResult == end(downKeys))
				return GetKeyUpTranslationResult(singleButton);
		}
		return {};
	}

	// This is the reset translation
	[[nodiscard]]
	inline
	auto GetButtonTranslationForUpToInitial(CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		// if the timer has elapsed, update back to the initial state.
		if(singleButton.LastAction.IsUp() && singleButton.LastAction.LastSentTime.IsElapsed())
		{
			return GetResetTranslationResult(singleButton);
		}
		return {};
	}

	/**
	 * \brief Encapsulates the mapping buffer, processes wrapped controller state updates, returns translation packs.
	 * \remarks If, before destruction, the mappings are in a state other than initial or awaiting reset, then you may wish to
	 *	make use of the <c>GetCleanupActions()</c> function. Not copyable. Is movable.
	 *	<p></p>
	 *	<p>An invariant exists such that: <b>There must be only one mapping per virtual keycode.</b></p>
	 */
	template<ValidFilterType OvertakingFilter_t = KeyboardOvertakingFilter>
	class KeyboardTranslator final
	{
		using MappingVector_t = std::vector<CBActionMap>;
		static_assert(MappingRange_c<MappingVector_t>);
		MappingVector_t m_mappings;
		std::optional<OvertakingFilter_t> m_filter;
	public:
		KeyboardTranslator() = delete; // no default
		KeyboardTranslator(const KeyboardTranslator& other) = delete; // no copy
		auto operator=(const KeyboardTranslator& other)->KeyboardTranslator & = delete; // no copy-assign
		~KeyboardTranslator() = default;

		KeyboardTranslator(KeyboardTranslator&& other) noexcept // implemented move
			: m_mappings(std::move(other.m_mappings)), m_filter(other.m_filter)
		{
		}

		auto operator=(KeyboardTranslator&& other) noexcept -> KeyboardTranslator& // implemented move-assign
		{
			if (this == &other)
				return *this;
			m_mappings = std::move(other.m_mappings);
			m_filter = other.m_filter;
			return *this;
		}

		/**
		 * \brief Mapping Vector move Ctor, throws on exclusivity group error, initializes the timers with the custom timer values.
		 * \param keyMappings Forwarding ref to a mapping vector type.
		 * \exception std::runtime_error on exclusivity group error during construction
		 */
		explicit KeyboardTranslator(MappingVector_t&& keyMappings )
		: m_mappings(std::move(keyMappings))
		{
			for (auto& e : m_mappings)
				InitCustomTimers(e);
			if (!AreMappingsUniquePerVk(m_mappings) || !AreMappingVksNonZero(m_mappings))
				throw std::runtime_error("Exception: More than 1 mapping per VK!");
		}

		// Ctor for adding a filter.
		KeyboardTranslator(MappingVector_t&& keyMappings, OvertakingFilter_t&& filter)
			: m_mappings(std::move(keyMappings)), m_filter(std::move(filter))
		{
			for (auto& e : m_mappings)
				InitCustomTimers(e);
			if (!AreMappingsUniquePerVk(m_mappings) || !AreMappingVksNonZero(m_mappings))
				throw std::runtime_error("Exception: More than 1 mapping per VK!");
			m_filter->SetMappingRange(m_mappings);
		}
	public:
		[[nodiscard]]
		auto operator()(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate) noexcept -> TranslationPack
		{
			return m_filter.has_value() ? GetUpdatedState(m_filter->GetFilteredButtonState(std::move(stateUpdate))) : GetUpdatedState(std::move(stateUpdate));
		}

		[[nodiscard]]
		auto GetUpdatedState(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate) noexcept -> TranslationPack
		{
			TranslationPack translations;
			for(std::size_t i{}; i < m_mappings.size(); ++i)
			{
				auto& mapping = m_mappings[i];
				if (const auto upToInitial = GetButtonTranslationForUpToInitial(mapping))
				{
					translations.UpdateRequests.emplace_back(*upToInitial);
				}
				else if (const auto initialToDown = GetButtonTranslationForInitialToDown(stateUpdate, mapping))
				{
					// Advance to next state.
					translations.DownRequests.emplace_back(*initialToDown);
				}
				else if (const auto downToFirstRepeat = GetButtonTranslationForDownToRepeat(stateUpdate, mapping))
				{
					translations.RepeatRequests.emplace_back(*downToFirstRepeat);
				}
				else if (const auto repeatToRepeat = GetButtonTranslationForRepeatToRepeat(stateUpdate, mapping))
				{
					translations.RepeatRequests.emplace_back(*repeatToRepeat);
				}
				else if (const auto repeatToUp = GetButtonTranslationForDownOrRepeatToUp(stateUpdate, mapping))
				{
					translations.UpRequests.emplace_back(*repeatToUp);
				}
			}
			return translations;
		}

		[[nodiscard]]
		auto GetCleanupActions() noexcept -> keyboardtypes::SmallVector_t<TranslationResult>
		{
			keyboardtypes::SmallVector_t<TranslationResult> translations;
			for(auto & mapping : m_mappings)
			{
				if(DoesMappingNeedCleanup(mapping.LastAction))
				{
					translations.emplace_back(GetKeyUpTranslationResult(mapping));
				}
			}
			return translations;
		}
	};

	static_assert(InputPoller_c<KeyboardTranslator<>>);
	static_assert(std::movable<KeyboardTranslator<>>);

}