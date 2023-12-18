module;

export module TranslationResult;

import CustomTypes;
import MappingStateTracker;
import VirtualController;
import ButtonMapping;
import <span>;
import <ranges>;
import <algorithm>;
import <functional>;
import <optional>;
import <numeric>;

export namespace sds
{
	/**
	 * \brief	TranslationResult holds info from a translated state change, typically the operation to perform (if any) and
	 *		a function to call to advance the state to the next state to continue to receive proper translation results.
	 *	\remarks	The advance state function can be used to ensure the operation to perform occurs BEFORE the mapping advances it's state.
	 *		This does mean that it may be possible to induce some error related to setting the state inappropriately. Without this
	 *		design, it would be possible to, for instance, withhold calling the operation to perform, yet still have the mapping's state updating internally, erroneously.
	 *		I believe this will make calling order-dependent functionality less error-prone.
	 */
	struct TranslationResult final
	{
		// TODO test with std::unique_ptr to Fn_t, it currently is like 18k of stack space.
		// Operation being requested to be performed, callable
		Fn_t OperationToPerform;
		// Function to advance the button mapping to the next state (after operation has been performed)
		Fn_t AdvanceStateFn;
		// Hash of the mapping it refers to
		VirtualButtons MappingVk{};
		// Exclusivity grouping value, if any
		OptGrp_t ExclusivityGrouping;
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

		SmallVector_t<TranslationResult> UpRequests{}; // key-ups
		SmallVector_t<TranslationResult> DownRequests{}; // key-downs
		SmallVector_t<TranslationResult> RepeatRequests{}; // repeats
		SmallVector_t<TranslationResult> UpdateRequests{}; // resets
		// TODO might wrap the vectors in a struct with a call operator to have individual call operators for range of TranslationResult.
	};

	static_assert(std::copyable<TranslationPack>);
	static_assert(std::movable<TranslationPack>);

	[[nodiscard]]
	inline auto GetResetTranslationResult(MappingContainer& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]()
			{
				if (currentMapping.StateFunctions.OnReset)
					currentMapping.StateFunctions.OnReset();
			},
			.AdvanceStateFn = [&currentMapping]()
			{
				currentMapping.LastAction.SetInitial();
				currentMapping.LastAction.LastSentTime.Reset();
			},
			.MappingVk = currentMapping.Button.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.Button.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline auto GetRepeatTranslationResult(MappingContainer& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]()
			{
				if (currentMapping.StateFunctions.OnRepeat)
					currentMapping.StateFunctions.OnRepeat();
				currentMapping.LastAction.LastSentTime.Reset();
			},
			.AdvanceStateFn = [&currentMapping]() 
			{
				currentMapping.LastAction.SetRepeat();
			},
			.MappingVk = currentMapping.Button.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.Button.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline auto GetOvertakenTranslationResult(MappingContainer& overtakenMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&overtakenMapping]()
			{
				if (overtakenMapping.StateFunctions.OnUp)
					overtakenMapping.StateFunctions.OnUp();
			},
			.AdvanceStateFn = [&overtakenMapping]()
			{
				overtakenMapping.LastAction.SetUp();
			},
			.MappingVk = overtakenMapping.Button.ButtonVirtualKeycode,
			.ExclusivityGrouping = overtakenMapping.Button.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline auto GetKeyUpTranslationResult(MappingContainer& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]()
			{
				if (currentMapping.StateFunctions.OnUp)
					currentMapping.StateFunctions.OnUp();
			},
			.AdvanceStateFn = [&currentMapping]()
			{
				currentMapping.LastAction.SetUp();
			},
			.MappingVk = currentMapping.Button.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.Button.ExclusivityGrouping
		};
	}

	[[nodiscard]]
	inline auto GetInitialKeyDownTranslationResult(MappingContainer& currentMapping) noexcept -> TranslationResult
	{
		return TranslationResult
		{
			.OperationToPerform = [&currentMapping]()
			{
				if (currentMapping.StateFunctions.OnDown)
					currentMapping.StateFunctions.OnDown();
				// Reset timer after activation, to wait for elapsed before another next state translation is returned.
				currentMapping.LastAction.LastSentTime.Reset();
				currentMapping.LastAction.DelayBeforeFirstRepeat.Reset();
			},
			.AdvanceStateFn = [&currentMapping]()
			{
				currentMapping.LastAction.SetDown();
			},
			.MappingVk = currentMapping.Button.ButtonVirtualKeycode,
			.ExclusivityGrouping = currentMapping.Button.ExclusivityGrouping
		};
	}

	/**
	 * \brief	Checks a list of mappings for having multiple mappings mapped to a single controller button.
	 * \param	mappingsList Span of controller button to action mappings.
	 * \return	true if good (or empty) mapping list, false if there is a problem.
	 */
	[[nodiscard]]
	inline bool AreMappingsUniquePerVk(const std::span<const MappingContainer> mappingsList) noexcept
	{
		SmallFlatMap_t<VirtualButtons, bool> mappingTable;
		for (const auto& e : mappingsList)
		{
			if (mappingTable[e.Button.ButtonVirtualKeycode])
			{
				return false;
			}
			mappingTable[e.Button.ButtonVirtualKeycode] = true;
		}
		return true;
	}

	/**
	 * \brief	Checks a list of mappings for having multiple mappings mapped to a single controller button.
	 * \param	mappingsList Span of controller button to action mappings.
	 * \return	true if good (or empty) mapping list, false if there is a problem.
	 */
	[[nodiscard]]
	inline bool AreMappingVksNonZero(const std::span<const MappingContainer> mappingsList) noexcept
	{
		return !std::ranges::any_of(mappingsList, [](const auto vk) { return vk.ButtonVirtualKeycode == VirtualButtons::NotSet; }, &MappingContainer::Button);
	}
}