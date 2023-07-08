#pragma once
#include "KeyboardLibIncludes.h"
#include "ControllerButtonToActionMap.h"

#include <optional>
#include <vector>
#include <ostream>
#include <type_traits>
#include <span>
#include <algorithm>

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
		std::size_t MappingHash{};
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
	 * \brief	TranslationPack is a pack of ranges containing individual TranslationResult structs for processing
	 *	state changes.
	 */
	struct TranslationPack final
	{
		void operator()() const
		{
			// Note that there will be a function called if there is a state change,
			// it just may not have any custom behavior attached to it.
			for (const auto& elem : UpdateRequests)
				elem();
			for (const auto& elem : RepeatRequests)
				elem();
			for (const auto& elem : NextStateRequests)
				elem();
		}
		// TODO might wrap the vectors in a struct with a call operator to have individual call operators for range of TranslationResult.
		keyboardtypes::SmallVector_t<TranslationResult> UpdateRequests{};
		keyboardtypes::SmallVector_t<TranslationResult> RepeatRequests{};
		//keyboardtypes::SmallVector_t<TranslationResult> OvertakenRequests{};
		keyboardtypes::SmallVector_t<TranslationResult> NextStateRequests{};
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
			.MappingHash = hash_value(currentMapping),
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
			.MappingHash = hash_value(currentMapping),
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
			.MappingHash = hash_value(overtakenMapping),
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
			.MappingHash = hash_value(currentMapping),
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
			.MappingHash = hash_value(currentMapping),
			.ExclusivityGrouping = currentMapping.ExclusivityGrouping
		};
	}

	/**
	 * \brief	Checks a list of mappings for having multiple exclusivity groupings mapped to a single controller button.
	 * \param	mappingsList Vector of controller button to action mappings.
	 * \return	true if good mapping list, false if there is a problem.
	 */
	[[nodiscard]]
	inline
	bool AreExclusivityGroupsUnique(const std::span<CBActionMap> mappingsList) noexcept
	{
		std::map<keyboardtypes::VirtualKey_t, keyboardtypes::OptGrp_t> groupMap;
		for (const auto& e : mappingsList)
		{
			// If an exclusivity group is set, we must verify no duplicate ex groups are set to the same vk
			const auto& vk = e.ButtonVirtualKeycode;
			const auto& currentMappingGroupOpt = e.ExclusivityGrouping;
			const auto& existingGroupOpt = groupMap[vk];
			if (currentMappingGroupOpt.has_value() && existingGroupOpt.has_value())
			{
				if (existingGroupOpt.value() != currentMappingGroupOpt.value())
					return false;
			}
			groupMap[vk] = currentMappingGroupOpt;
		}
		return true;
	}

}
