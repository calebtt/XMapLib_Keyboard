#pragma once
#include <stdexcept>
#include "KeyboardLibIncludes.h"
#include "KeyboardCustomTypes.h"
#include "ControllerStateUpdateWrapper.h"
#include "KeyboardTranslationHelpers.h"

/*
 *	Note: There are some static sized arrays used here with capacity defined in customtypes.
 */

namespace sds
{
	template<typename Poller_t>
	concept InputPoller_c = requires(Poller_t & t)
	{
		{ t.GetUpdatedState(ControllerStateUpdateWrapper<>{{}}) } -> std::convertible_to<TranslationPack>;
	};
	// Concept for range of CBActionMap type that at least provides one-time forward-iteration.
	template<typename T>
	concept MappingRange_c = requires (T & t)
	{
		{ std::same_as<typename T::value_type, CBActionMap> };
		{ std::ranges::forward_range<T> };
	};

	/*
	 *	NOTE: Testing these functions may be quite easy, pass a single CBActionMap in a certain state to all of these functions,
	 *	and if more than one TranslationResult is produced (aside from perhaps the reset translation), then it would obviously be in error.
	 */

	/**
	 * \brief For a single mapping, search the controller state update buffer and produce a TranslationResult appropriate to the current mapping state and controller state.
	 * \param updatesWrapper Wrapper class containing the results of a controller state update polling.
	 * \param singleButton The mapping type for a single virtual key of the controller.
	 * \returns Optional, <c>TranslationResult</c>
	 */
	[[nodiscard]]
	inline
	auto GetButtonTranslationForInitialToDown(const ControllerStateUpdateWrapper<>& updatesWrapper, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		if (singleButton.LastAction.IsInitialState())
		{
			const auto downResults = GetDownVirtualKeycodesRange(updatesWrapper);
			const auto findResult = find(downResults, singleButton.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the down translation.
			if(findResult != end(downResults))
				return GetInitialKeyDownTranslationResult(singleButton);
		}
		return {};
	}

	[[nodiscard]]
	inline
	auto GetButtonTranslationForDownToRepeat(const ControllerStateUpdateWrapper<>& updatesWrapper, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		const bool isDownAndUsesRepeat = singleButton.LastAction.IsDown() && (singleButton.UsesInfiniteRepeat || singleButton.SendsFirstRepeatOnly);
		if (isDownAndUsesRepeat && singleButton.LastAction.DelayBeforeFirstRepeat.IsElapsed())
		{
			const auto downResults = GetDownVirtualKeycodesRange(updatesWrapper);
			const auto findResult = find(downResults, singleButton.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the repeat translation.
			if (findResult != end(downResults))
				return GetRepeatTranslationResult(singleButton);
		}
		return {};
	}

	[[nodiscard]]
	inline
	auto GetButtonTranslationForRepeatToRepeat(const ControllerStateUpdateWrapper<>& updatesWrapper, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		const bool isRepeatAndUsesInfinite = singleButton.LastAction.IsRepeating() && singleButton.UsesInfiniteRepeat;
		if (isRepeatAndUsesInfinite && singleButton.LastAction.LastSentTime.IsElapsed())
		{
			const auto downResults = GetDownVirtualKeycodesRange(updatesWrapper);
			const auto findResult = find(downResults, singleButton.ButtonVirtualKeycode);
			// If VK *is* found in the down list, create the repeat translation.
			if (findResult != end(downResults))
				return GetRepeatTranslationResult(singleButton);
		}
		return {};
	}

	[[nodiscard]]
	inline
	auto GetButtonTranslationForDownOrRepeatToUp(const ControllerStateUpdateWrapper<>& updatesWrapper, CBActionMap& singleButton) noexcept -> std::optional<TranslationResult>
	{
		using std::ranges::find, std::ranges::end;
		if (singleButton.LastAction.IsDown() || singleButton.LastAction.IsRepeating())
		{
			const auto downResults = GetDownVirtualKeycodesRange(updatesWrapper);
			const auto findResult = find(downResults, singleButton.ButtonVirtualKeycode);
			// If VK is not found in the down list, create the up translation.
			if(findResult == end(downResults))
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
	 */
	class KeyboardPollerControllerLegacy final
	{
		using MappingVector_t = std::vector<CBActionMap>;
		static_assert(MappingRange_c<MappingVector_t>);
		MappingVector_t m_mappings;
	public:
		KeyboardPollerControllerLegacy() = delete; // no default
		KeyboardPollerControllerLegacy(const KeyboardPollerControllerLegacy& other) = delete; // no copy

		/**
		 * \brief Move constructor will call the cleanup actions on the moved-into instance before the move!
		 */
		KeyboardPollerControllerLegacy(KeyboardPollerControllerLegacy&& other) noexcept // implemented move
		{
			m_mappings = std::move(other.m_mappings);
		}

		auto operator=(const KeyboardPollerControllerLegacy& other) -> KeyboardPollerControllerLegacy& = delete; // no copy-assign

		auto operator=(KeyboardPollerControllerLegacy&& other) noexcept -> KeyboardPollerControllerLegacy& // implemented move-assign
		{
			if (this == &other)
				return *this;
			m_mappings = std::move(other.m_mappings);
			return *this;
		}

		/**
		 * \brief Mapping Vector move Ctor, throws on exclusivity group error, initializes the timers with the custom timer values.
		 * \param keyMappings Forwarding ref to a mapping vector type.
		 * \exception std::runtime_error on exclusivity group error during construction
		 */
		explicit KeyboardPollerControllerLegacy(MappingVector_t&& keyMappings )
		: m_mappings(std::move(keyMappings))
		{
			for (auto& e : m_mappings)
				InitCustomTimers(e);
			if (!AreExclusivityGroupsUnique(m_mappings))
				throw std::runtime_error("Exception: Mappings with multiple exclusivity groupings for a single VK!");
		}

		/**
		 * \brief Mapping Vector copy Ctor, throws on exclusivity group error, initializes the timers with the custom timer values.
		 * \param keyMappings const-ref to an iterable range of mappings.
		 * \exception std::runtime_error on exclusivity group error during construction
		 */
		explicit KeyboardPollerControllerLegacy(const MappingRange_c auto& keyMappings)
		: m_mappings(keyMappings)
		{
			for (auto& e : m_mappings)
				InitCustomTimers(e);
			if (!AreExclusivityGroupsUnique(m_mappings))
				throw std::runtime_error("Exception: Mappings with multiple exclusivity groupings for a single VK!");
		}

		~KeyboardPollerControllerLegacy() = default;
	public:
		[[nodiscard]]
		auto operator()(const ControllerStateUpdateWrapper<>& stateUpdate) noexcept -> TranslationPack
		{
			return GetUpdatedState(stateUpdate);
		}

		[[nodiscard]]
		auto GetUpdatedState(const ControllerStateUpdateWrapper<>& stateUpdate) noexcept -> TranslationPack
		{
			TranslationPack translations;
			for(std::size_t i{}; i < m_mappings.size(); ++i)
			{
				auto& mapping = m_mappings[i];
				if (const auto upToInitial = GetButtonTranslationForUpToInitial(mapping))
				{
					translations.UpdateRequests.emplace_back(*upToInitial);
					continue;
				}
				if (const auto initialToDown = GetButtonTranslationForInitialToDown(stateUpdate, mapping))
				{
					// Advance to next state.
					translations.NextStateRequests.emplace_back(*initialToDown);
					continue;
				}
				if (const auto downToFirstRepeat = GetButtonTranslationForDownToRepeat(stateUpdate, mapping))
				{
					translations.NextStateRequests.emplace_back(*downToFirstRepeat);
					continue;
				}
				if (const auto repeatToRepeat = GetButtonTranslationForRepeatToRepeat(stateUpdate, mapping))
				{
					translations.RepeatRequests.emplace_back(*repeatToRepeat);
					continue;
				}
				if (const auto repeatToUp = GetButtonTranslationForDownOrRepeatToUp(stateUpdate, mapping))
				{
					translations.NextStateRequests.emplace_back(*repeatToUp);
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
				if(const bool isLastActionDownOrRepeat = mapping.LastAction.IsDown() || mapping.LastAction.IsRepeating())
				{
					translations.emplace_back(GetKeyUpTranslationResult(mapping));
				}
			}
			return translations;
		}

		// To provide read-only access to the internal mappings buffer.
		[[nodiscard]]
		auto GetMappingsRef() const noexcept -> const MappingVector_t&
		{
			return m_mappings;
		}
	};

	static_assert(InputPoller_c<KeyboardPollerControllerLegacy>);
	static_assert(std::movable<KeyboardPollerControllerLegacy>);

}