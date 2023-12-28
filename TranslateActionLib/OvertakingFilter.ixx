module;

export module OvertakingFilter;


import <cassert>;
import <ranges>;
import <functional>;
import <numeric>;
import <execution>;
import <deque>;
import <format>;
import <source_location>;
import <stacktrace>;
import <span>;
import <map>;

import CustomTypes;
import VirtualController;
import ButtonMapping;
import TranslationResult;
import MappingRangeAlgos;
import Translator;

export namespace sds
{
	template<typename GroupInfo_t>
	concept FilterGroupInfo_c = requires(GroupInfo_t & t)
	{
		{ t.IsMappingActivated(VirtualButtons::A) } -> std::convertible_to<bool>;
		{ t.IsMappingOvertaken(VirtualButtons::A) } -> std::convertible_to<bool>;
		{ t.IsAnyMappingActivated()	} -> std::convertible_to<bool>;
		{ t.IsMappingActivatedOrOvertaken(VirtualButtons::A) } -> std::convertible_to<bool>;
		{ t.GetActivatedValue() } -> std::convertible_to<VirtualButtons>;
		{ t.UpdateForNewMatchingGroupingDown(VirtualButtons::A) } -> std::convertible_to<std::pair<bool, std::optional<VirtualButtons>>>;
		{ t.UpdateForNewMatchingGroupingUp(VirtualButtons::A) } -> std::convertible_to<std::optional<VirtualButtons>>;
	};

	/**
	 * \brief	<para>A logical representation of a mapping's exclusivity group activation status, for this setup a single key in the exclusivity group can be 'activated'
	 *	or have a key-down state at a time. It is exclusively the only key in the group forwarded to the translator for processing of key-down events.</para>
	 * <para>Essentially this is used to ensure only a single key per exclusivity grouping is down at a time, and keys can overtake the current down key. </para>
	 * \remarks This abstraction manages the currently activated key being "overtaken" by another key from the same group and causing a key-up/down to be sent for the currently activated,
	 *	as well as moving the key in line behind the newly activated key. A much needed abstraction.
	 */
	class GroupActivationInfo final
	{
		using Elem_t = VirtualButtons;

		// First element of the queue is the activated mapping.
		std::deque<Elem_t> ActivatedValuesQueue;
	public:
		/**
		 * \brief Boolean of the returned pair is whether or not the keydown should be filtered/removed.
		 *	The optional value is (optionally) referring to the mapping to send a new key-up for.
		 * \remarks An <b>precondition</b> is that the mapping's value passed into this has a matching exclusivity grouping!
		 */
		[[nodiscard]] auto UpdateForNewMatchingGroupingDown(const Elem_t newDownVk) noexcept -> std::pair<bool, std::optional<Elem_t>>
		{
			// Filter all of the hashes already activated/overtaken.
			const bool isActivated = IsMappingActivated(newDownVk);
			const bool isOvertaken = IsMappingOvertaken(newDownVk);
			const bool doFilterTheDown = isOvertaken;
			if (isActivated || isOvertaken)
				return std::make_pair(doFilterTheDown, std::optional<Elem_t>{});

			// If any mapping hash is already activated, this new hash will be overtaking it and thus require a key-up for current activated.
			if (IsAnyMappingActivated())
			{
				const auto currentDownValue = ActivatedValuesQueue.front();
				ActivatedValuesQueue.push_front(newDownVk);
				return std::make_pair(false, currentDownValue);
			}

			// New activated mapping case, add to queue in first position and don't filter. No key-up required.
			ActivatedValuesQueue.push_front(newDownVk);
			return std::make_pair(false, std::optional<Elem_t>{});
		}

		/**
		 * \brief The optional value is (optionally) referring to the mapping to send a new key-down for,
		 *	in the event that the currently activated key is key-up'd and there is an overtaken key waiting behind it in the queue.
		 * \remarks An <b>precondition</b> is that the mapping passed into this has a matching exclusivity grouping!
		 */
		auto UpdateForNewMatchingGroupingUp(const Elem_t newUpVk) noexcept -> std::optional<Elem_t>
		{
			// Handle no hashes in queue to update case, and specific new up hash not in queue either.
			if (!IsAnyMappingActivated())
				return {};

			const auto findResult = std::ranges::find(ActivatedValuesQueue, newUpVk);
			const bool isFound = findResult != std::ranges::cend(ActivatedValuesQueue);

			if (isFound)
			{
				const bool isInFirstPosition = findResult == ActivatedValuesQueue.cbegin();

				// Case wherein the currently activated mapping is the one getting a key-up.
				if (isInFirstPosition)
				{
					if (ActivatedValuesQueue.size() > 1)
					{
						// If there is an overtaken queue, key-down the next key in line.
						ActivatedValuesQueue.pop_front();
						// Return the new front hash to be sent a key-down.
						return ActivatedValuesQueue.front();
					}
				}

				// otherwise, just remove it from the queue because it hasn't been key-down'd (it's one of the overtaken, or size is 1).
				ActivatedValuesQueue.erase(findResult);
			}

			return {};
		}

	public:
		[[nodiscard]] bool IsMappingActivated(const Elem_t vk) const noexcept
		{
			if (ActivatedValuesQueue.empty())
				return false;
			return vk == ActivatedValuesQueue.front();
		}
		[[nodiscard]] bool IsMappingOvertaken(const Elem_t vk) const noexcept
		{
			if (ActivatedValuesQueue.empty())
				return false;

			const bool isCurrentActivation = ActivatedValuesQueue.front() == vk;
			const auto findResult = std::ranges::find(ActivatedValuesQueue, vk);
			const bool isFound = findResult != std::ranges::cend(ActivatedValuesQueue);
			return !isCurrentActivation && isFound;
		}
		[[nodiscard]] bool IsAnyMappingActivated() const noexcept { return !ActivatedValuesQueue.empty(); }
		[[nodiscard]] bool IsMappingActivatedOrOvertaken(const Elem_t vk) const noexcept
		{
			const auto findResult = std::ranges::find(ActivatedValuesQueue, vk);
			return findResult != std::ranges::cend(ActivatedValuesQueue);
		}
		[[nodiscard]] auto GetActivatedValue() const noexcept -> Elem_t
		{
			assert(!ActivatedValuesQueue.empty());
			return ActivatedValuesQueue.front();
		}
	};
	static_assert(std::movable<GroupActivationInfo>);
	static_assert(std::copyable<GroupActivationInfo>);

	/**
	 * \brief	May be used to internally filter the poller's translations in order to apply the overtaking behavior.
	 * \remarks This behavior is deviously complex, and modifications are best done to "GroupActivationInfo" only, if at all possible.
	 *	In the event that a single state update contains presently un-handled key-downs for mappings with the same exclusivity grouping,
	 *	it will only process a single overtaking key-down at a time, and will suppress the rest in the state update to be handled on the next iteration.
	 */
	template<typename GroupInfo_t = GroupActivationInfo>
	class OvertakingFilter final
	{
		using VirtualCode_t = VirtualButtons;

		// Mapping of exclusivity grouping value to 
		using MapType_t = std::map<GrpVal_t, GroupInfo_t>;

		// span to mappings
		std::span<const MappingContainer> m_mappings;

		// map of grouping value to GroupActivationInfo container.
		MapType_t m_groupMap;
	public:
		OvertakingFilter() = delete;

		explicit OvertakingFilter(const Translator& translator)
		{
			SetMappingRange(translator.GetMappingsRange());
		}

		// This function is used to filter the controller state updates before they are sent to the translator.
		// It will have an effect on overtaking behavior by modifying the state update buffer, which just contains the virtual keycodes that are reported as down.
		[[nodiscard]] auto GetFilteredButtonState(SmallVector_t<VirtualCode_t>&& stateUpdate) -> SmallVector_t<VirtualCode_t>
		{
			using std::ranges::sort;

			// Sorting provides an ordering to which down states with an already handled exclusivity grouping get filtered out for this iteration.
			//sort(stateUpdate, std::ranges::less{}); // TODO <-- problem for the (current) unit testing, optional anyway

			stateUpdate = FilterStateUpdateForUniqueExclusivityGroups(std::move(stateUpdate));

			auto filteredForDown = FilterDownTranslation(stateUpdate);

			// There appears to be no reason to report additional VKs that will become 'down' after a key is moved to up,
			// because for the key to still be in the overtaken queue, it would need to still be 'down' as well, and thus handled
			// by the down filter.
			FilterUpTranslation(stateUpdate);

			return filteredForDown;
		}

		auto operator()(SmallVector_t<VirtualCode_t>&& stateUpdate) -> SmallVector_t<VirtualCode_t>
		{
			return GetFilteredButtonState(std::move(stateUpdate));
		}
	private:

		void SetMappingRange(const std::span<const MappingContainer> mappingsList)
		{
			m_mappings = mappingsList;
			m_groupMap = {};

			// Build the map of ex. group information.
			for (const auto& elem : mappingsList)
			{
				if (elem.Button.ExclusivityGrouping)
				{
					const auto grpVal = *elem.Button.ExclusivityGrouping;
					m_groupMap[grpVal] = {};
				}
			}
		}

		[[nodiscard]] auto FilterDownTranslation(const SmallVector_t<VirtualCode_t>& stateUpdate) -> SmallVector_t<VirtualCode_t>
		{
			using std::ranges::find;
			using std::ranges::cend;
			using std::ranges::views::transform;
			using std::ranges::views::filter;
			using std::erase;

			auto stateUpdateCopy = stateUpdate;

			// filters for all mappings of interest per the current 'down' VK buffer.
			const auto vkHasMappingPred = [this](const auto vk) -> bool
				{
					const auto findResult = GetMappingByVk(vk, m_mappings);
					return findResult.has_value();
				};
			const auto exGroupPred = [this](const auto ind) -> bool
				{
					return GetMappingAt(ind).Button.ExclusivityGrouping.has_value();
				};
			const auto mappingIndexPred = [this](const auto vk) -> std::size_t
				{ 
					auto findResult = GetMappingIndexForVk(vk, m_mappings);
					assert(findResult.has_value());
					return *findResult;
				};

			SmallVector_t<VirtualCode_t> vksToRemoveRange;

			for (const auto index : stateUpdateCopy | filter(vkHasMappingPred) | transform(mappingIndexPred) | filter(exGroupPred))
			{
				const auto& currentMapping = GetMappingAt(index);
				auto& currentGroup = m_groupMap[*currentMapping.Button.ExclusivityGrouping];

				const auto& [shouldFilter, upOpt] = currentGroup.UpdateForNewMatchingGroupingDown(currentMapping.Button.ButtonVirtualKeycode);
				if (shouldFilter)
				{
					vksToRemoveRange.push_back(currentMapping.Button.ButtonVirtualKeycode);
				}
				if (upOpt)
				{
					vksToRemoveRange.push_back(*upOpt);
				}
			}

			EraseValuesFromRange(stateUpdateCopy, vksToRemoveRange);

			return stateUpdateCopy;
		}

		// it will process only one key per ex. group per iteration. The others will be filtered out and handled on the next iteration.
		void FilterUpTranslation(const SmallVector_t<VirtualCode_t>& stateUpdate)
		{
			using std::ranges::views::filter;

			// filters for all mappings of interest per the current 'down' VK buffer (the UP mappings in this case).
			const auto exGroupPred = [](const auto& currentMapping) 
				{ 
					return currentMapping.Button.ExclusivityGrouping.has_value();
				};
			const auto stateUpdateUpPred = [&stateUpdate](const auto& currentMapping) 
				{ 
					return !IsMappingInRange(currentMapping.Button.ButtonVirtualKeycode, stateUpdate); 
				};

			for (const auto& currentMapping : m_mappings | filter(exGroupPred) | filter(stateUpdateUpPred))
			{
				auto& currentGroup = m_groupMap[*currentMapping.Button.ExclusivityGrouping];
				currentGroup.UpdateForNewMatchingGroupingUp(currentMapping.Button.ButtonVirtualKeycode);
			}
		}

	private:
		[[nodiscard]] constexpr auto GetMappingAt(const std::size_t index) noexcept -> const MappingContainer&
		{
			return m_mappings[index];
		}

		/**
		 * \brief Used to remove VKs with an exclusivity grouping that another state update VK already has. Processed from begin to end, so the first processed VK will be the left-most and
		 *	duplicates to the right will be removed.
		 * \remarks This is essential because processing more than one exclusivity grouping having mapping in a single iteration of a filter will mean the first ex. group vks were not actually processed
		 *	by the translator, yet their state would be updated by the filter incorrectly. Also, VKs in the state update must be unique! One VK per mapping is a hard precondition.
		 * \return "filtered" state update.
		 */
		[[nodiscard]] auto FilterStateUpdateForUniqueExclusivityGroups(SmallVector_t<VirtualCode_t>&& stateUpdate) -> SmallVector_t<VirtualCode_t>
		{
			using std::ranges::find_if, std::ranges::cend, std::ranges::find;
			using StateRange_t = std::remove_cvref_t<decltype(stateUpdate)>;

			SmallVector_t<GrpVal_t> groupingValueBuffer;
			StateRange_t virtualKeycodesToRemove;
			groupingValueBuffer.reserve(stateUpdate.size());
			virtualKeycodesToRemove.reserve(stateUpdate.size());

			for (const auto vk : stateUpdate)
			{
				const auto foundMappingForVk = find_if(m_mappings, [vk](const auto& e) 
					{
						return e.Button.ButtonVirtualKeycode == vk;
					});
				if (foundMappingForVk != cend(m_mappings))
				{
					if (foundMappingForVk->Button.ExclusivityGrouping)
					{
						const auto grpVal = foundMappingForVk->Button.ExclusivityGrouping.value();
						auto& currentGroup = m_groupMap[grpVal];
						if (!currentGroup.IsMappingActivatedOrOvertaken(vk))
						{
							const auto groupingFindResult = find(groupingValueBuffer, grpVal);

							// If already in located, being handled groupings, add to remove buffer.
							if (groupingFindResult != cend(groupingValueBuffer))
								virtualKeycodesToRemove.push_back(vk);
							// Otherwise, add this new grouping to the grouping value buffer.
							else
								groupingValueBuffer.push_back(grpVal);
						}
					}
				}
			}

			EraseValuesFromRange(stateUpdate, virtualKeycodesToRemove);

			return stateUpdate;
		}
	};
	static_assert(std::copyable<OvertakingFilter<>>);
	static_assert(std::movable<OvertakingFilter<>>);

	/**
	 * \brief	Filter concept, used to apply a specific "overtaking" behavior (exclusivity grouping behavior) implementation.
	 */
	template<typename FilterType_t>
	concept ValidFilterType_c = requires(FilterType_t & t)
	{
		{ t.SetMappingRange(std::span<MappingContainer>{}) };
		{ t.GetFilteredButtonState({ VirtualButtons::A, VirtualButtons::X, VirtualButtons::Y }) } -> std::convertible_to<SmallVector_t<VirtualButtons>>;
		{ std::movable<FilterType_t> == true };
	};
}