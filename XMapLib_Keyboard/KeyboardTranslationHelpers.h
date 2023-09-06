#pragma once
#include "KeyboardCustomTypes.h"
#include "ControllerButtonToActionMap.h"

#include <optional>
#include <vector>
#include <ostream>
#include <type_traits>
#include <span>
#include <algorithm>
#include <concepts>
#include <map>

namespace sds
{
	/**
	 * \brief	TranslationResult holds info from a translated state change, typically the operation to perform (if any) and
	 *	a function to call to advance the state to the next state to continue to receive proper translation results.
	 */
	struct TranslationResult final
	{
		// Operation being requested to be performed, callable
		keyboardtypes::Fn_t OperationToPerform;
		// Function to advance the button mapping to the next state (after operation has been performed)
		keyboardtypes::Fn_t AdvanceStateFn;
		// Hash of the mapping it refers to
		keyboardtypes::VirtualKey_t MappingVk{};
		// Exclusivity grouping value, if any
		keyboardtypes::OptGrp_t ExclusivityGrouping;
		// Call operator, calls op fn then advances the state
		void operator()() const
		{
			OperationToPerform();
			AdvanceStateFn();
		}
	};

	static_assert(std::copyable<TranslationResult>);
	static_assert(std::movable<TranslationResult>);

	/**
	 * \brief	TranslationPack is a pack of ranges containing individual TranslationResult structs for processing state changes.
	 * \remarks		If using the provided call operator, it will prioritize key-up requests, then key-down requests, then repeat requests, then updates.
	 *	I figure it should process key-ups and new key-downs with the highest priority, after that keys doing a repeat, and lastly updates.
	 */
	struct TranslationPack final
	{
		void operator()() const
		{
			// Note that there will be a function called if there is a state change,
			// it just may not have any custom behavior attached to it.
			for (const auto& elem : UpRequests)
				elem();
			for (const auto& elem : DownRequests)
				elem();
			for (const auto& elem : RepeatRequests)
				elem();
			for (const auto& elem : UpdateRequests)
				elem();
		}

		keyboardtypes::SmallVector_t<TranslationResult> UpRequests{}; // key-ups
		keyboardtypes::SmallVector_t<TranslationResult> DownRequests{}; // key-downs
		keyboardtypes::SmallVector_t<TranslationResult> RepeatRequests{}; // repeats
		keyboardtypes::SmallVector_t<TranslationResult> UpdateRequests{}; // resets
		// TODO might wrap the vectors in a struct with a call operator to have individual call operators for range of TranslationResult.
	};

	static_assert(std::copyable<TranslationPack>);
	static_assert(std::movable<TranslationPack>);

	/**
	 * \brief	Initializes the MappingStateManager timers with custom time delays from the mapping.
	 * \param mappingElem	The controller button to action mapping, possibly with the optional custom delay values.
	 */
	inline
	void InitCustomTimers(CBActionMap& mappingElem) noexcept
	{
		if (mappingElem.DelayForRepeats)
			mappingElem.LastAction.LastSentTime.Reset(mappingElem.DelayForRepeats.value());
		if (mappingElem.DelayBeforeFirstRepeat)
			mappingElem.LastAction.DelayBeforeFirstRepeat.Reset(mappingElem.DelayBeforeFirstRepeat.value());
	}

	[[nodiscard]]
	inline
	auto GetResetTranslationResult(CBActionMap& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]() {
				if (currentMapping.OnReset)
					currentMapping.OnReset();
			},
			.AdvanceStateFn = [&currentMapping]() {
				currentMapping.LastAction.SetInitial();
				currentMapping.LastAction.LastSentTime.Reset();
			},
			.MappingVk = currentMapping.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline
	auto GetRepeatTranslationResult(CBActionMap& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]() {
				if (currentMapping.OnRepeat)
					currentMapping.OnRepeat();
				currentMapping.LastAction.LastSentTime.Reset();
			},
			.AdvanceStateFn = [&currentMapping]() {
				currentMapping.LastAction.SetRepeat();
			},
			.MappingVk = currentMapping.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline
	auto GetOvertakenTranslationResult(CBActionMap& overtakenMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&overtakenMapping]()
			{
				if (overtakenMapping.OnUp)
					overtakenMapping.OnUp();
			},
			.AdvanceStateFn = [&overtakenMapping]()
			{
				overtakenMapping.LastAction.SetUp();
			},
			.MappingVk = overtakenMapping.ButtonVirtualKeycode,
			.ExclusivityGrouping = overtakenMapping.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline
	auto GetKeyUpTranslationResult(CBActionMap& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]()
			{
				if (currentMapping.OnUp)
					currentMapping.OnUp();
			},
			.AdvanceStateFn = [&currentMapping]()
			{
				currentMapping.LastAction.SetUp();
			},
			.MappingVk = currentMapping.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline
	auto GetInitialKeyDownTranslationResult(CBActionMap& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]()
			{
				if (currentMapping.OnDown)
					currentMapping.OnDown();
				// Reset timer after activation, to wait for elapsed before another next state translation is returned.
				currentMapping.LastAction.LastSentTime.Reset();
				currentMapping.LastAction.DelayBeforeFirstRepeat.Reset();
			},
			.AdvanceStateFn = [&currentMapping]()
			{
				currentMapping.LastAction.SetDown();
			},
			.MappingVk = currentMapping.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.ExclusivityGrouping
		};
	}

	/**
	 * \brief	Checks a list of mappings for having multiple mappings mapped to a single controller button.
	 * \param	mappingsList Span of controller button to action mappings.
	 * \return	true if good (or empty) mapping list, false if there is a problem.
	 */
	[[nodiscard]]
	inline
	bool AreMappingsUniquePerVk(const std::span<const CBActionMap> mappingsList) noexcept
	{
		keyboardtypes::SmallFlatMap_t<keyboardtypes::VirtualKey_t, bool> mappingTable;
		for(const auto& e : mappingsList)
		{
			if(mappingTable[e.ButtonVirtualKeycode])
			{
				return false;
			}
			mappingTable[e.ButtonVirtualKeycode] = true;
		}
		return true;
	}

	/**
	 * \brief	Checks a list of mappings for having multiple mappings mapped to a single controller button.
	 * \param	mappingsList Span of controller button to action mappings.
	 * \return	true if good (or empty) mapping list, false if there is a problem.
	 */
	[[nodiscard]]
	inline
	bool AreMappingVksNonZero(const std::span<const CBActionMap> mappingsList) noexcept
	{
		return ! std::ranges::any_of(mappingsList, [](const auto vk) { return vk == 0; }, &CBActionMap::ButtonVirtualKeycode);
	}
}
