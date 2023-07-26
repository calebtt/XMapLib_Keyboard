#pragma once
#include <cassert>

#include "KeyboardCustomTypes.h"
#include "ControllerButtonToActionMap.h"
#include "KeyboardTranslationHelpers.h"

namespace sds
{
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
	constexpr
	auto GetGroupInfoForUnsetHashcode(const GroupActivationInfo& existingGroup) -> GroupActivationInfo
	{
		return GroupActivationInfo{ existingGroup.GroupingValue, {}, {} };
	}

	[[nodiscard]]
	constexpr
	auto GetGroupInfoForNewSetHashcode(const GroupActivationInfo& existingGroup, const std::size_t newlyActivatedHash)
	{
		return GroupActivationInfo{ existingGroup.GroupingValue, newlyActivatedHash, {} };
	}


	/**
	 * \brief	May be used to internally filter the poller's translations in order to apply the overtaking behavior.
	 */
	class OvertakingFilter final
	{
		// Constructed from the mapping list, pairs with ex. group data.
		keyboardtypes::SmallVector_t<GroupActivationInfo> m_exclusivityGroupInfo;
		// span to mappings
		std::span<CBActionMap> m_mappings;
	public:
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

		auto FilterDownTranslation(TranslationResult&& translation) -> FilteredPair
		{
			using
			std::ranges::find,
			std::ranges::find_if,
			std::ranges::cend;

			if(!translation.ExclusivityGrouping)
			{
				return { .Original = std::move(translation), .Overtaking = {} };
			}

			const auto groupIndex = GetGroupingInfoIndex(*translation.ExclusivityGrouping);
			assert(groupIndex.has_value());
			const auto foundResult = find(m_exclusivityGroupInfo[*groupIndex].OvertakenHashes, translation.MappingHash);
			const bool isTranslationAlreadyOvertaken = foundResult != cend(m_exclusivityGroupInfo[*groupIndex].OvertakenHashes);
			const bool isTranslationAlreadyActivated = m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash == translation.MappingHash;
			const bool isNoMappingHashSet = !m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash;
			const auto currentMapping = find_if(m_mappings, [&](const auto& elem)
				{
					const auto elemHash = hash_value(elem);
					return translation.MappingHash == elemHash;
				});
			assert(currentMapping != cend(m_mappings));

			FilteredPair filteredPair;
			// Handle new down
			if(isNoMappingHashSet)
			{
				UpdateGroupInfoForNewDown(translation, *groupIndex);
				filteredPair.Original = std::move(translation);
			}
			// Handle overtaking down translation, not one of the already overtaken.
			else if(!isTranslationAlreadyActivated && !isTranslationAlreadyOvertaken)
			{
				const auto overtakenMappingIndex = GetMappingIndexByHash(m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash);
				UpdateGroupInfoForOvertakingDown(translation, *groupIndex);
				filteredPair.Overtaking = GetKeyUpTranslationResult(m_mappings[overtakenMappingIndex]);
				filteredPair.Original = std::move(translation);
			}
			return filteredPair;
		}

		auto FilterUpTranslation(const TranslationResult& translation)
		{
			using std::ranges::find,
			std::ranges::end,
			std::ranges::find_if,
			std::ranges::cend;

			if(!translation.ExclusivityGrouping)
			{
				return;
			}

			const auto groupIndex = GetGroupingInfoIndex(*translation.ExclusivityGrouping);
			assert(groupIndex.has_value());
			const auto foundResult = find(m_exclusivityGroupInfo[*groupIndex].OvertakenHashes, translation.MappingHash);
			const bool isTranslationAlreadyOvertaken = foundResult != cend(m_exclusivityGroupInfo[*groupIndex].OvertakenHashes);
			const bool isTranslationAlreadyActivated = m_exclusivityGroupInfo[*groupIndex].ActivatedMappingHash == translation.MappingHash;

			if(isTranslationAlreadyActivated)
			{
				// Remove from activated slot, clear items from overtaken buffer
				UpdateGroupInfoForUp(*groupIndex);
			}
			else if(isTranslationAlreadyOvertaken)
			{
				// Removed from overtaken buffer
				RemoveItemFromOvertakenGroup(*groupIndex, translation.MappingHash);
			}
		}
	private:
		[[nodiscard]]
		constexpr
		auto GetMappingIndexByHash(const std::size_t hash) const -> std::size_t
		{
			for(std::size_t i{}; i < m_mappings.size(); ++i)
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
			for(std::size_t i{}; i < m_exclusivityGroupInfo.size(); ++i)
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
			m_exclusivityGroupInfo[groupIndex].OvertakenHashes.emplace_back(m_exclusivityGroupInfo[groupIndex].ActivatedMappingHash);
			m_exclusivityGroupInfo[groupIndex].ActivatedMappingHash = translation.MappingHash;
			m_exclusivityGroupInfo[groupIndex].GroupingValue = m_exclusivityGroupInfo[groupIndex].GroupingValue;
		}

		// Handle up translation with matching hash-code set as activated (unset the activated grouping hash-code)
		void UpdateGroupInfoForUp(const std::size_t groupIndex)
		{
			m_exclusivityGroupInfo[groupIndex] = GetGroupInfoForUnsetHashcode(m_exclusivityGroupInfo[groupIndex]);
		}

		void RemoveItemFromOvertakenGroup(const std::size_t groupIndex, const std::size_t mappingHash)
		{
			using std::ranges::find,
				std::ranges::end,
				std::ranges::find_if,
				std::ranges::cend;
			auto foundResult = find(m_exclusivityGroupInfo[groupIndex].OvertakenHashes, mappingHash);
			assert(foundResult != cend(m_exclusivityGroupInfo[groupIndex].OvertakenHashes));
			m_exclusivityGroupInfo[groupIndex].OvertakenHashes.erase(foundResult);
		}
	};
}