#pragma once
#include <cassert>
#include <ranges>
#include <functional>
#include <numeric>
#include <iostream>
#include <format>
#include <execution>
#include "ControllerButtonToActionMap.h"
#include "KeyboardTranslationHelpers.h"

namespace sds
{
	using VkToMappingsTable_t = keyboardtypes::SmallFlatMap_t<keyboardtypes::VirtualKey_t, keyboardtypes::SmallVector_t<keyboardtypes::Index_t>>;

	/**
	 * \brief	A logical representation of a mapping's exclusivity group activation status.
	 */
	struct GroupActivationInfo final
	{
		// Exclusivity grouping value, mirroring the mapping value used.
		keyboardtypes::GrpVal_t GroupingValue{};
		// A value of 0 indicates no mapping is activated.
		std::size_t ActivatedMappingHash{};
		// Necessary to prevent switching between down/up repeatedly.
		keyboardtypes::SmallVector_t<std::size_t> OvertakenHashes;
	};

	struct FilteredPair
	{
		std::optional<TranslationResult> Original;
		std::optional<TranslationResult> Overtaking;
	};

	[[nodiscard]]
	constexpr
	auto GetActivatedGroupingInfo(const std::span<GroupActivationInfo> groupRange, const std::integral auto groupValue, const std::integral auto hashToMatch)
	{
		using std::ranges::find_if, std::ranges::cend;
		const auto findResult = find_if(groupRange, [groupValue, hashToMatch](const GroupActivationInfo& e)
			{
				return e.ActivatedMappingHash == hashToMatch && e.GroupingValue == groupValue;
			});
		return std::make_pair(findResult != cend(groupRange), findResult);
	}

	[[nodiscard]]
	inline
	auto GetGroupInfoForUnsetHashcode(const GroupActivationInfo& existingGroup) -> GroupActivationInfo
	{
		return GroupActivationInfo{ existingGroup.GroupingValue, {}, {} };
	}

	[[nodiscard]]
	inline
	auto GetGroupInfoForNewSetHashcode(const GroupActivationInfo& existingGroup, const std::size_t newlyActivatedHash)
	{
		return GroupActivationInfo{ existingGroup.GroupingValue, newlyActivatedHash, {} };
	}

	/**
	 * \brief Returns the indices at which a mapping that matches the 'vk' was found.
	 * \param vk Virtual keycode of the presumably 'down' key with which to match CBActionMap mappings.
	 * \param mappingsRange The range of CBActionMap mappings for which to return the indices of matching mappings.
	 */
	[[nodiscard]]
	inline
	auto GetMappingIndexForVk(const keyboardtypes::VirtualKey_t vk, const std::span<const CBActionMap> mappingsRange) -> keyboardtypes::Index_t
	{
		using std::ranges::find;
		using std::ranges::cend;
		using std::ranges::cbegin;
		using std::ranges::distance;
		using keyboardtypes::Index_t;

		const auto findResult = find(mappingsRange, vk, &CBActionMap::ButtonVirtualKeycode);
		assert(findResult != cend(mappingsRange));
		return static_cast<Index_t>(distance(cbegin(mappingsRange), findResult));

		//using std::ranges::views::enumerate, std::ranges::views::filter;
		//const auto matchingVkFilter = [vk](const auto elem)
		//	{
		//		return std::get<1>(elem).ButtonVirtualKeycode == vk;
		//	};
		//// build list of mappings that match the down VK
		//keyboardtypes::SmallVector_t<keyboardtypes::Index_t> mappingIndices;
		//for (const auto [ind, elem] : mappingsRange | enumerate | filter(matchingVkFilter))
		//{
		//	mappingIndices.emplace_back(static_cast<keyboardtypes::Index_t>(ind));
		//}

		//return mappingIndices;
	}

	/**
	 * \brief Used to retrieve information to access the range of CBActionMap mappings with a matching virtual key specified in keyRange.
	 * \param keyRange Range of virtual key codes with which to match to added mappings.
	 * \param mappingsRange The range of mappings describing controller button VK to state change actions.
	 */
	//[[nodiscard]]
	//inline
	//auto GetMappingIndicesForVks(const std::span<const keyboardtypes::VirtualKey_t> keyRange, const std::span<const CBActionMap> mappingsRange) -> VkToMappingsTable_t
	//{
	//	using std::ranges::find, std::ranges::cend, std::ranges::cbegin, std::ranges::distance;

	//	// Map of VK to mapping indices.
	//	VkToMappingsTable_t vkToIndicesMap;

	//	std::ranges::for_each(keyRange.begin(), keyRange.end(), [&vkToIndicesMap, &mappingsRange](const auto vk)
	//	{
	//		vkToIndicesMap[vk] = GetMappingIndexForVk(vk, mappingsRange);
	//	});

	//	return vkToIndicesMap;
	//}


	[[nodiscard]]
	inline
	auto RemoveDuplicateExclusivityGroupIndices(std::span<const keyboardtypes::VirtualKey_t> indexRange)
	{
		using std::ranges::sort, std::ranges::copy;
		using std::ranges::views::chunk_by, std::ranges::not_equal_to;

		// create a copy
		keyboardtypes::SmallVector_t<uint32_t> indices(indexRange.size(), 0);

		copy(indexRange.cbegin(), indexRange.cend(), indices.begin());

		// sort it
		sort(indices, std::less<uint32_t>{});

		// chunk it and then test each sub-range for having an exclusivity group, if so, take only the lowest value.
		for(const auto duplicatesRange : indices | chunk_by(not_equal_to{}))
		{
			if(duplicatesRange.size() > 1)
			{
				//std::cout << std::ranges::views::all(duplicatesRange) << '\n';
			}
		}

		//if(!std::is_sorted(indices.cbegin(), indices.cend()))
		//{
		//	std::cerr << "Not sorted.\n";
		//}




		// 
		// TODO for this implementation, we will just use the order in which the mappings were added to the array as the priority.
		// TODO actually, best to just use the virtual keycode lowest to highest


		// TODO to handle multiple exclusivity group holding mapping changes at the same time requires some kind of strict priority ordering imposed on the
		// TODO input polling results.

		// TODO as in, if someone presses two exclusivity grouping having buttons at the exact same time and a single state update contains both, which is performed?
		// TODO the arbitrary one that came first in the stream of 'down' virtual keycodes buffer? Then, to change that, someone would have to update GetDownVirtualKeycodesRange(...)
	}

	/**
	 * \brief	May be used to internally filter the poller's translations in order to apply the overtaking behavior.
	 */
	class KeyboardOvertakingFilter final
	{
		// Constructed from the mapping list, pairs with ex. group data.
		keyboardtypes::SmallVector_t<GroupActivationInfo> m_exclusivityGroupInfo;
		// span to mappings
		std::span<CBActionMap> m_mappings;
		KeyboardSettingsPack m_settings;
	public:

		// This function is used to filter the controller state updates before they are sent to the translator.
		// It will effect overtaking behavior by modifying the state update buffer, which just contains the virtual keycodes that are reported as down.
		// TODO complete this
		auto GetFilteredButtonState(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate) -> keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>
		{
			using std::ranges::views::filter;
			using std::ranges::cend;
			using std::ranges::find;
			using std::ranges::find_if;

			FilterDownTranslation(stateUpdate);

			return stateUpdate;

			//// TODO filter and keep track of exclusivity groupings based on a stream of down/up virtual key-code information, before being
			//// sent on to the normal functionality translator. Behavior for this type of thing can be extremely nuanced and it should be
			//// possible for anyone else to make a custom filter such as this.

			//const auto existsAndHasGroup = [&stateUpdate](const CBActionMap& e)
			//{
			//	const auto foundResult = find(stateUpdate, e.ButtonVirtualKeycode);
			//	return foundResult != cend(stateUpdate) && e.ExclusivityGrouping.has_value();
			//};
			//const auto newDownFilter = [](const CBActionMap& e) { return e.LastAction.IsInitialState(); };
			//const auto alreadyDownFilter = [](const CBActionMap& e) { return e.LastAction.IsDown() || e.LastAction.IsRepeating(); };

			//auto filteredStateUpdate = m_mappings | filter(newDownFilter) | filter(existsAndHasGroup);

			//const auto DownVkWithGroupFilter = [maps = m_mappings](const keyboardtypes::VirtualKey_t keyCode)
			//{
			//	const auto findResult = find_if(maps, [keyCode](const CBActionMap& e)
			//		{
			//			return e.ExclusivityGrouping.has_value() && e.ButtonVirtualKeycode == keyCode;
			//		});
			//	return findResult != cend(maps);
			//};
			//const auto NewDownVkFilter = [maps = m_mappings](const keyboardtypes::VirtualKey_t keyCode)
			//{
			//	const auto findResult = find_if(maps, [keyCode](const CBActionMap& e) { return e.ButtonVirtualKeycode == keyCode; });
			//	return findResult != cend(maps) && findResult->LastAction.IsInitialState();
			//};

			//auto downKeysWithGrouping = stateUpdate | filter(DownVkWithGroupFilter);
			//auto newDownKeys = stateUpdate | filter(NewDownVkFilter) | filter(DownVkWithGroupFilter);

			//// Handle new down keys.
			//for(const auto elem: newDownKeys)
			//{
			//	const auto mappingIndex = GetMappingIndexByVk(elem);
			//	auto& currentMapping = m_mappings[mappingIndex];
			//	
			//}

			//for(auto& elem : filteredStateUpdate)
			//{
			//	std::cout << std::format("{}\n", elem.ExclusivityGrouping.value_or(0));
			//	// TODO might build a map or something, add to m_exgroupinfo data member for a new down.
			//	// TODO probably have 3 functions, one for new down, one for repeating down, one for up. Don't know what I will do with the repeat-down but someone else might.
			//	//FilterDownTranslation(elem);
			//}
			//// TODO parse all of the down key VKs based on overtaking behavior.
			//return stateUpdate;
		}

		auto FilterDownTranslation(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t> stateUpdate)
		{
			using std::ranges::sort, std::ranges::copy;
			using std::ranges::find;
			using std::ranges::find_if;
			using std::ranges::cend;
			using std::ranges::count_if;
			using std::ranges::views::transform;
			using std::ranges::views::chunk_by;

			// indices for all mappings of interest per the current 'down' VK buffer.
			const auto indices = GetMappingIndicesForVks(stateUpdate, m_mappings);

			for(const auto& [vk, indexBuf] : indices)
			{
				// for each sub-range of mapping indices pertaining to a single VK.

				const auto mappingsView = indexBuf | transform([this](const auto ind) -> const CBActionMap&
				{
					return GetMappingAt(ind);
				});
				// Count the number of mappings with the same ex. group
				// todo the result of this chunk is a sub-range of the ex grouping values, not the mappings.
				const auto chunkedByGroup = mappingsView | transform([](const auto& e) { return e.ExclusivityGrouping.value_or(0); }) | chunk_by(std::ranges::not_equal_to{});
				// Do some logic for the ex. group chunked sub-ranges
				//for(const auto mappingSubRange : chunkedByGroup)
				//{
				//	// If current sub-range has more than one element, and has an ex. grouping
				//	if(mappingSubRange.size() > 1 && mappingSubRange.front().ExclusivityGrouping.has_value())
				//	{
				//		
				//	}

				//}
				//const auto countedResult = count_if(chunkedByGroup, [&](const auto& elem) { *std::get<1>(elem).ExclusivityGrouping == *currentMapping.ExclusivityGrouping; });
			}

			//const auto groupIndex = GetGroupingInfoIndex(*translation.ExclusivityGrouping);
			//assert(groupIndex.has_value());
			//const auto foundResult = find(m_exclusivityGroupInfo[*groupIndex].OvertakenHashes, translation.MappingHash);
			//const bool isTranslationAlreadyOvertaken = foundResult != cend(m_exclusivityGroupInfo[*groupIndex].OvertakenHashes);
			//const bool isTranslationAlreadyActivated = m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash == translation.MappingHash;
			//const bool isNoMappingHashSet = !m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash;
			//const auto currentMapping = find_if(m_mappings, [&](const auto& elem)
			//	{
			//		const auto elemHash = hash_value(elem);
			//		return translation.MappingHash == elemHash;
			//	});
			//assert(currentMapping != cend(m_mappings));

			//FilteredPair filteredPair;
			//// Handle new down
			//if (isNoMappingHashSet)
			//{
			//	UpdateGroupInfoForNewDown(translation, *groupIndex);
			//	filteredPair.Original = std::move(translation);
			//}
			//// Handle overtaking down translation, not one of the already overtaken.
			//else if (!isTranslationAlreadyActivated && !isTranslationAlreadyOvertaken)
			//{
			//	const auto overtakenMappingIndex = GetMappingIndexByHash(m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash);
			//	UpdateGroupInfoForOvertakingDown(translation, *groupIndex);
			//	filteredPair.Overtaking = GetKeyUpTranslationResult(m_mappings[overtakenMappingIndex]);
			//	filteredPair.Original = std::move(translation);
			//}
			//return filteredPair;
		}

		auto FilterUpTranslation(const TranslationResult& translation) -> FilteredPair
		{
			using std::ranges::find,
				std::ranges::end,
				std::ranges::find_if,
				std::ranges::cend;

			if (!translation.ExclusivityGrouping)
				return { .Original = translation };

			const auto groupIndex = GetGroupingInfoIndex(*translation.ExclusivityGrouping);
			assert(groupIndex.has_value());
			auto& currentGroupInfo = m_exclusivityGroupInfo[*groupIndex];

			const auto foundResult = find(currentGroupInfo.OvertakenHashes, translation.MappingHash);
			const bool isTranslationAlreadyOvertaken = foundResult != cend(currentGroupInfo.OvertakenHashes);
			const bool isTranslationAlreadyActivated = currentGroupInfo.ActivatedMappingHash == translation.MappingHash;
			const bool hasWaitingOvertaken = !currentGroupInfo.OvertakenHashes.empty();

			FilteredPair filteredUp{ .Original = translation };
			if (isTranslationAlreadyActivated)
			{
				// Remove from activated slot, replace with next in line
				currentGroupInfo.ActivatedMappingHash = {};
				if (hasWaitingOvertaken)
				{
					currentGroupInfo.ActivatedMappingHash = *currentGroupInfo.OvertakenHashes.cbegin();
					currentGroupInfo.OvertakenHashes.erase(currentGroupInfo.OvertakenHashes.cbegin());
					// Prepare key-down for it.
					const auto newDownMappingIndex = GetMappingIndexByHash(currentGroupInfo.ActivatedMappingHash);
					filteredUp.Overtaking = GetInitialKeyDownTranslationResult(m_mappings[newDownMappingIndex]);
				}
			}
			else if (isTranslationAlreadyOvertaken)
			{
				// Does not require sending a key-up for these.
				currentGroupInfo.OvertakenHashes.erase(foundResult);
			}

			return filteredUp;
		}

		void SetMappingRange(std::span<CBActionMap> mappingsList)
		{
			m_mappings = mappingsList;
			m_exclusivityGroupInfo = {};
			// Build the buffer of ex. group information.
			for (const auto& elem : mappingsList)
			{
				if (elem.ExclusivityGrouping)
				{
					m_exclusivityGroupInfo.emplace_back(
						GroupActivationInfo
						{
							.GroupingValue = *elem.ExclusivityGrouping,
							.ActivatedMappingHash = 0,
							.OvertakenHashes = {}
						});
				}
			}
		}
	private:
		[[nodiscard]]
		constexpr
		auto GetMappingAt(const std::size_t index) noexcept -> CBActionMap&
		{
			return m_mappings[index];
		}

		[[nodiscard]]
		constexpr
		auto GetMappingIndexByVk(const keyboardtypes::VirtualKey_t keyCode) const -> std::size_t
		{
			for (std::size_t i{}; i < m_mappings.size(); ++i)
			{
				if (hash_value(m_mappings[i]) == keyCode)
					return i;
			}
			assert(false);
			return {};
		}

		[[nodiscard]]
		constexpr
		auto GetMappingIndexByHash(const std::size_t hash) const -> std::size_t
		{
			for (std::size_t i{}; i < m_mappings.size(); ++i)
			{
				if (hash_value(m_mappings[i]) == hash)
					return i;
			}
			assert(false);
			return {};
		}

		[[nodiscard]]
		auto GetGroupingInfoIndex(const std::integral auto exclusivityGroupValue) -> std::optional<std::size_t>
		{
			for (std::size_t i{}; i < m_exclusivityGroupInfo.size(); ++i)
			{
				if (m_exclusivityGroupInfo[i].GroupingValue == exclusivityGroupValue)
					return i;
			}
			return {};
		}

		// Handle not activated grouping (set hash-code as activated for the grouping)
		void UpdateGroupInfoForNewDown(const TranslationResult& translation, const std::size_t groupIndex)
		{
			m_exclusivityGroupInfo[groupIndex] = GetGroupInfoForNewSetHashcode(m_exclusivityGroupInfo[groupIndex], translation.MappingHash);
		}

		// Handle overtaking down translation
		void UpdateGroupInfoForOvertakingDown(const TranslationResult& translation, const std::size_t groupIndex)
		{
			auto& currentRange = m_exclusivityGroupInfo[groupIndex].OvertakenHashes;
			currentRange.emplace_back(0);
			std::ranges::shift_right(currentRange, 1);
			currentRange[0] = m_exclusivityGroupInfo[groupIndex].ActivatedMappingHash;
			m_exclusivityGroupInfo[groupIndex].ActivatedMappingHash = translation.MappingHash;
			m_exclusivityGroupInfo[groupIndex].GroupingValue = m_exclusivityGroupInfo[groupIndex].GroupingValue;
		}

		// Handle up translation with matching hash-code set as activated (unset the activated grouping hash-code)
		void UpdateGroupInfoForUp(const std::size_t groupIndex)
		{
			m_exclusivityGroupInfo[groupIndex] = GetGroupInfoForUnsetHashcode(m_exclusivityGroupInfo[groupIndex]);
		}

		// Handle up translation with matching hash-code set as activated--but also more than one mapping in the overtaken buffer.
		void UpdateGroupInfoForUpOfActivatedHash(const std::size_t groupIndex)
		{
			using std::ranges::begin,
				std::ranges::cbegin,
				std::ranges::end,
				std::ranges::cend,
				std::ranges::remove;

			auto& currentGroupInfo = m_exclusivityGroupInfo[groupIndex];
			GroupActivationInfo newGroup{
				.GroupingValue = currentGroupInfo.GroupingValue,
				.ActivatedMappingHash = {},
				.OvertakenHashes = {}
			};

			auto& currentOvertakenBuffer = currentGroupInfo.OvertakenHashes;
			const auto firstElement = cbegin(currentOvertakenBuffer);
			if (firstElement != cend(currentOvertakenBuffer))
			{
				newGroup.ActivatedMappingHash = *firstElement;
				currentOvertakenBuffer.erase(firstElement);
				newGroup.OvertakenHashes = currentOvertakenBuffer;
			}

			//auto& currentGroup = m_exclusivityGroupInfo[groupIndex];
			//auto tempGroupInfo = GetGroupInfoForUnsetHashcode(currentGroup);
			//const bool isOvertakenBufferEmpty = currentGroup.OvertakenHashes.empty();
			//if(!isOvertakenBufferEmpty)
			//{
			//	const auto hashedValueForFirstInLine = *currentGroup.OvertakenHashes.begin();
			//	currentGroup.OvertakenHashes.erase(currentGroup.OvertakenHashes.begin());
			//	tempGroupInfo.ActivatedMappingHash = hashedValueForFirstInLine;
			//}
			//tempGroupInfo.OvertakenHashes = m_exclusivityGroupInfo[groupIndex].OvertakenHashes;
			//m_exclusivityGroupInfo[groupIndex] = tempGroupInfo;
		}
	};

}
