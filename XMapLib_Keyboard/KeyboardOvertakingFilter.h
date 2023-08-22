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
	}

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

			stateUpdate = FilterDownTranslation(stateUpdate);

			return stateUpdate;
		}

		auto FilterDownTranslation(const keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t> stateUpdate)
		{
			using std::ranges::find;
			using std::ranges::cend;
			using std::ranges::views::transform;
			using std::ranges::views::filter;
			using std::erase;

			auto filteredCopy = stateUpdate;
			// indices for all mappings of interest per the current 'down' VK buffer.
			auto indices = stateUpdate | transform([this](const auto vk) { return GetMappingIndexForVk(vk, m_mappings); }) | filter([this](const auto ind) { return GetMappingAt(ind).ExclusivityGrouping.has_value(); });

			for(const auto index : indices)
			{
				const auto& currentMapping = GetMappingAt(index);
				const auto foundGroupInfo = find(m_exclusivityGroupInfo, *currentMapping.ExclusivityGrouping, &GroupActivationInfo::GroupingValue);
				auto& currentGroup = *foundGroupInfo;
				assert(foundGroupInfo != cend(m_exclusivityGroupInfo));

				const auto currentMappingHash = hash_value(currentMapping);
				const bool isAnyHashActivated = currentGroup.ActivatedMappingHash != 0;
				const bool isCurrentMappingActivated = currentGroup.ActivatedMappingHash != currentMappingHash;
				const bool isCurrentMappingOvertaken = find(currentGroup.OvertakenHashes, currentMappingHash) != cend(currentGroup.OvertakenHashes);
				// TODO complete this
				if(!isAnyHashActivated)
				{
					currentGroup.ActivatedMappingHash = currentMappingHash;
				}

				if(isAnyHashActivated && !isCurrentMappingOvertaken)
				{
					currentGroup.OvertakenHashes.emplace_back(currentMappingHash);
					filteredCopy.erase(find(filteredCopy, currentMapping.ButtonVirtualKeycode));
				}
			}
			return filteredCopy;
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
		auto GetMappingAt(const std::size_t index) noexcept -> const CBActionMap&
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
