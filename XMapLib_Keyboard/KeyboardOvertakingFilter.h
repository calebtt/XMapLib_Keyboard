#pragma once
#include <cassert>
#include <ranges>
#include <functional>
#include <numeric>
#include <iostream>
#include <format>
#include <execution>
#include <deque>
#include "ControllerButtonToActionMap.h"
#include "KeyboardTranslationHelpers.h"

namespace sds
{
	//using VkToMappingsTable_t = keyboardtypes::SmallFlatMap_t<keyboardtypes::VirtualKey_t, keyboardtypes::SmallVector_t<keyboardtypes::Index_t>>;

	/**
	 * \brief	A logical representation of a mapping's exclusivity group activation status, for this setup a single key
	 *	in the exclusivity group can be 'activated' or have a key-down state at a time. It is exclusively the only key in the group set to down.
	 * \remarks This abstraction manages the currently activated key being "overtaken" by another key from the same group and causing a key-up to be sent for the currently activated,
	 *	as well as moving the key in line behind the newly activated key. A much needed abstraction.
	 */
	class GroupActivationInfo final
	{
	public:
		// Exclusivity grouping value, mirroring the mapping value used.
		keyboardtypes::GrpVal_t GroupingValue{};
	private:
		// First element of the queue is the activated mapping.
		std::deque<std::size_t> ActivatedHashes;
	public:
		/**
		 * \brief Bool value of the returned pair is whether or not the keydown should be filtered/removed.
		 *	The optional size_t is (optionally) a hash of the mapping to send a new key-up for.
		 * \remarks An <b>precondition</b> is that the mapping passed into this matches the exclusivity grouping!
		 */
		[[nodiscard]] auto UpdateForNewMatchingGroupingDown(const std::size_t newDownHash) noexcept -> std::pair<bool, std::optional<std::size_t>>
		{
			// Filter all of the hashes already activated/overtaken.
			if (IsMappingActivatedOrOvertaken(newDownHash))
				return std::make_pair(true, std::optional<std::size_t>{});

			// If any mapping hash is already activated, this new hash will be overtaking it and thus require a key-up for current activated.
			if(IsAnyMappingActivated())
			{
				const auto returnedDownHash = ActivatedHashes.front();
				ActivatedHashes.push_front(newDownHash);
				return std::make_pair(false, returnedDownHash);
			}

			// New activated mapping case, add to queue in first position and don't filter. No key-up required.
			ActivatedHashes.push_front(newDownHash);
			return std::make_pair(false, std::optional<std::size_t>{});
		}

		/**
		 * \brief The optional size_t is (optionally) a hash of the mapping to send a new key-down for,
		 *	in the event that the currently activated key is key-up'd and there is an overtaken key waiting behind it in the queue.
		 * \remarks An <b>precondition</b> is that the mapping passed into this matches the exclusivity grouping!
		 */
		[[nodiscard]] auto UpdateForNewMatchingGroupingUp(const std::size_t newUpHash) noexcept -> std::optional<std::size_t>
		{
			// Handle no hashes in queue to update case, and specific new up hash not in queue either.
			if (!IsAnyMappingActivated() || !IsMappingActivatedOrOvertaken(newUpHash))
				return {};

			const auto findResult = std::ranges::find(ActivatedHashes, newUpHash);
			const bool isFound = findResult != std::ranges::cend(ActivatedHashes);

			if (isFound)
			{
				const bool isInFirstPosition = findResult == ActivatedHashes.cbegin();

				// Case wherein the currently activated mapping is the one getting a key-up.
				if (isInFirstPosition)
				{
					if (ActivatedHashes.size() > 1)
					{
						// If there is an overtaken queue, key-down the next key in line.
						ActivatedHashes.pop_front();
						// Return the new front hash to be sent a key-down.
						return ActivatedHashes.front();
					}
				}

				// otherwise, just remove it from the queue because it hasn't been key-down'd (it's one of the overtaken, or size is 1).
				ActivatedHashes.erase(findResult);
			}
			// TODO add unit test for this class!
			return {};
		}

		[[nodiscard]] bool IsMappingActivated(const std::size_t newHash) const noexcept
		{
			if (ActivatedHashes.empty())
				return false;
			return newHash == ActivatedHashes.front();
		}
		[[nodiscard]] bool IsMappingOvertaken(const std::size_t newHash) const noexcept
		{
			const bool isCurrentActivation = ActivatedHashes.front() == newHash;
			const auto findResult = std::ranges::find(ActivatedHashes, newHash);
			const bool isFound = findResult != std::ranges::cend(ActivatedHashes);
			return !isCurrentActivation && isFound;
		}
		[[nodiscard]] bool IsAnyMappingActivated() const noexcept { return !ActivatedHashes.empty(); }
		[[nodiscard]] bool IsMappingActivatedOrOvertaken(const std::size_t newHash) const noexcept
		{
			const auto findResult = std::ranges::find(ActivatedHashes, newHash);
			return findResult != std::ranges::cend(ActivatedHashes);
		}
		[[nodiscard]] auto GetGroupingValue() const noexcept -> keyboardtypes::GrpVal_t { return GroupingValue; }
		[[nodiscard]] auto GetActivatedHash() const noexcept -> std::size_t
		{
			assert(!ActivatedHashes.empty());
			return ActivatedHashes.front();
		}
	};

	//struct FilteredPair
	//{
	//	std::optional<TranslationResult> Original;
	//	std::optional<TranslationResult> Overtaking;
	//};

	//[[nodiscard]]
	//constexpr
	//auto GetActivatedGroupingInfo(const std::span<GroupActivationInfo> groupRange, const std::integral auto groupValue, const std::integral auto hashToMatch)
	//{
	//	using std::ranges::find_if, std::ranges::cend;
	//	const auto findResult = find_if(groupRange, [groupValue, hashToMatch](const GroupActivationInfo& e)
	//	{
	//		if (!e.IsAnyMappingActivated())
	//			return false;
	//		return e.GetActivatedHash() == hashToMatch && e.GetGroupingValue() == groupValue;
	//	});
	//	return std::make_pair(findResult != cend(groupRange), findResult);
	//}

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

	//[[nodiscard]]
	//inline
	//auto GetGroupInfoForUnsetHashcode(const GroupActivationInfo& existingGroup) -> GroupActivationInfo
	//{
	//	return GroupActivationInfo{ existingGroup.GetGroupingValue() };
	//}

	//[[nodiscard]]
	//inline
	//auto GetGroupInfoForNewSetHashcode(const GroupActivationInfo& existingGroup, const std::size_t newlyActivatedHash)
	//{
	//	return GroupActivationInfo
	//	{
	//		.GroupingValue = existingGroup.GetGroupingValue(),
	//		.ActivatedMappingHash = newlyActivatedHash
	//	};
	//}

	///**
	// * \brief Performs operations for suppressing a key-down VK in a state update buffer. Involves
	// *	erasing the VK from the state update buffer, and updating the GroupActivationInfo if necessary.
	// */
	//inline
	//auto SuppressKeyState(
	//	keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate, 
	//	const std::integral auto buttonVirtualKeycode, 
	//	GroupActivationInfo& currentGroup, 
	//	const std::integral auto currentMappingHash,
	//	const bool isInOvertakenRangeAlready) -> keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>
	//{
	//	using std::ranges::find;
	//	// TODO updating the group info will be handled via it's member funcs, and it will have encapsulated data.
	//	if(!isInOvertakenRangeAlready)
	//		currentGroup.OvertakenHashes.emplace_back(currentMappingHash);
	//	stateUpdate.erase(std::ranges::find(stateUpdate, buttonVirtualKeycode));
	//	return stateUpdate;
	//}

	//// Precondition is that VK for associated mapping reported a 'down'.
	//inline
	//auto UpdateGroupInfoFor(GroupActivationInfo& currentGroup, const CBActionMap& currentMapping)
	//{
	//	using std::ranges::find;
	//	using std::ranges::cend;
	//	using std::ranges::views::transform;
	//	using std::ranges::views::filter;
	//	using std::erase;

	//	const auto currentMappingHash = hash_value(currentMapping);
	//	const bool isAnyHashActivated = currentGroup.ActivatedMappingHash != 0;
	//	const bool isCurrentMappingActivated = currentGroup.ActivatedMappingHash != currentMappingHash;
	//	const bool isCurrentMappingOvertaken = find(currentGroup.OvertakenHashes, currentMappingHash) != cend(currentGroup.OvertakenHashes);
	//	// TODO complete this
	//	if (!isAnyHashActivated)
	//	{
	//		// Set as activated
	//		currentGroup.ActivatedMappingHash = currentMappingHash;
	//	}
	//	if (isCurrentMappingOvertaken)
	//	{
	//		// No updates to group info necessary
	//		return;
	//	}
	//	if (isAnyHashActivated && !isCurrentMappingActivated)
	//	{

	//	}
	//}

	//	// 
	//	// TODO for this implementation, we will just use the order in which the mappings were added to the array as the priority.
	//	// TODO actually, best to just use the virtual keycode lowest to highest


	//	// TODO to handle multiple exclusivity group holding mapping changes at the same time requires some kind of strict priority ordering imposed on the
	//	// TODO input polling results.

	//	// TODO as in, if someone presses two exclusivity grouping having buttons at the exact same time and a single state update contains both, which is performed?
	//	// TODO the arbitrary one that came first in the stream of 'down' virtual keycodes buffer? Then, to change that, someone would have to update GetDownVirtualKeycodesRange(...)

	/**
	 * \brief	May be used to internally filter the poller's translations in order to apply the overtaking behavior.
	 */
	class KeyboardOvertakingFilter final
	{
		// Mapping of exclusivity grouping value to 
		using MapType_t = keyboardtypes::SmallFlatMap_t<keyboardtypes::GrpVal_t, GroupActivationInfo>;

		// Constructed from the mapping list, pairs with ex. group data.
		//keyboardtypes::SmallVector_t<GroupActivationInfo> m_exclusivityGroupInfo;

		// span to mappings
		std::span<CBActionMap> m_mappings;
		KeyboardSettingsPack m_settings;
		MapType_t m_groupMap;
	public:
		void SetMappingRange(const std::span<CBActionMap> mappingsList)
		{
			m_mappings = mappingsList;
			m_groupMap = {};

			// Build the map of ex. group information.
			for (const auto& elem : mappingsList)
			{
				if (elem.ExclusivityGrouping)
				{
					const auto grpVal = *elem.ExclusivityGrouping;
					m_groupMap[grpVal].GroupingValue = grpVal;
				}
			}
		}

		// This function is used to filter the controller state updates before they are sent to the translator.
		// It will effect overtaking behavior by modifying the state update buffer, which just contains the virtual keycodes that are reported as down.
		// TODO complete this
		auto GetFilteredButtonState(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate) -> keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>
		{
			using std::ranges::views::filter;
			using std::ranges::cend;
			using std::ranges::find;
			using std::ranges::find_if;

			stateUpdate = FilterDownTranslation(std::move(stateUpdate));

			return stateUpdate;
		}

		auto FilterDownTranslation(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate)
		{
			using std::ranges::find;
			using std::ranges::cend;
			using std::ranges::views::transform;
			using std::ranges::views::filter;
			using std::erase;

			// filters for all mappings of interest per the current 'down' VK buffer.
			const auto exGroupPred = [this](const auto ind) { return GetMappingAt(ind).ExclusivityGrouping.has_value(); };
			const auto mappingIndexPred = [this](const auto vk) { return GetMappingIndexForVk(vk, m_mappings); };

			for(const auto index : stateUpdate | transform(mappingIndexPred) | filter(exGroupPred))
			{
				const auto& currentMapping = GetMappingAt(index);
				auto& currentGroup = m_groupMap[*currentMapping.ExclusivityGrouping];

				//UpdateGroupInfoFor(currentGroup, currentMapping);

				//const auto currentMappingHash = hash_value(currentMapping);
				//const bool isAnyHashActivated = currentGroup.ActivatedMappingHash != 0;
				//const bool isCurrentMappingActivated = currentGroup.ActivatedMappingHash != currentMappingHash;
				//const bool isCurrentMappingOvertaken = find(currentGroup.OvertakenHashes, currentMappingHash) != cend(currentGroup.OvertakenHashes);
			}
			return stateUpdate;
		}

		auto FilterUpTranslation(keyboardtypes::SmallVector_t<keyboardtypes::VirtualKey_t>&& stateUpdate)
		{
			using std::ranges::find;
			using std::ranges::cend;
			using std::ranges::views::transform;
			using std::ranges::views::filter;
			using std::erase;

			// filters for all mappings of interest per the current 'down' VK buffer.
			const auto exGroupPred = [this](const auto ind) { return GetMappingAt(ind).ExclusivityGrouping.has_value(); };
			const auto mappingIndexPred = [this](const auto vk) { return GetMappingIndexForVk(vk, m_mappings); };

			for (const auto index : stateUpdate | transform(mappingIndexPred) | filter(exGroupPred))
			{
				const auto& currentMapping = GetMappingAt(index);
				auto& currentGroup = m_groupMap[*currentMapping.ExclusivityGrouping];
			}
			//if (!translation.ExclusivityGrouping)
			//	return { .Original = translation };

			//const auto groupIndex = GetGroupingInfoIndex(*translation.ExclusivityGrouping);
			//assert(groupIndex.has_value());
			//auto& currentGroupInfo = m_exclusivityGroupInfo[*groupIndex];



			//const auto foundResult = find(currentGroupInfo.OvertakenHashes, translation.MappingHash);
			//const bool isTranslationAlreadyOvertaken = foundResult != cend(currentGroupInfo.OvertakenHashes);
			//const bool isTranslationAlreadyActivated = currentGroupInfo.ActivatedMappingHash == translation.MappingHash;
			//const bool hasWaitingOvertaken = !currentGroupInfo.OvertakenHashes.empty();

			//FilteredPair filteredUp{ .Original = translation };
			//if (isTranslationAlreadyActivated)
			//{
			//	// Remove from activated slot, replace with next in line
			//	currentGroupInfo.ActivatedMappingHash = {};
			//	if (hasWaitingOvertaken)
			//	{
			//		currentGroupInfo.ActivatedMappingHash = *currentGroupInfo.OvertakenHashes.cbegin();
			//		currentGroupInfo.OvertakenHashes.erase(currentGroupInfo.OvertakenHashes.cbegin());
			//		// Prepare key-down for it.
			//		const auto newDownMappingIndex = GetMappingIndexByHash(currentGroupInfo.ActivatedMappingHash);
			//		filteredUp.Overtaking = GetInitialKeyDownTranslationResult(m_mappings[newDownMappingIndex]);
			//	}
			//}
			//else if (isTranslationAlreadyOvertaken)
			//{
			//	// Does not require sending a key-up for these.
			//	currentGroupInfo.OvertakenHashes.erase(foundResult);
			//}

			//return filteredUp;
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

		//// Handle not activated grouping (set hash-code as activated for the grouping)
		//void UpdateGroupInfoForNewDown(const TranslationResult& translation, const std::size_t groupIndex)
		//{
		//	m_exclusivityGroupInfo[groupIndex] = GetGroupInfoForNewSetHashcode(m_exclusivityGroupInfo[groupIndex], translation.MappingHash);
		//}

		//// Handle overtaking down translation
		//void UpdateGroupInfoForOvertakingDown(const TranslationResult& translation, const std::size_t groupIndex)
		//{
		//	auto& currentRange = m_exclusivityGroupInfo[groupIndex].OvertakenHashes;
		//	currentRange.emplace_back(0);
		//	std::ranges::shift_right(currentRange, 1);
		//	currentRange[0] = m_exclusivityGroupInfo[groupIndex].ActivatedMappingHash;
		//	m_exclusivityGroupInfo[groupIndex].ActivatedMappingHash = translation.MappingHash;
		//	m_exclusivityGroupInfo[groupIndex].GroupingValue = m_exclusivityGroupInfo[groupIndex].GroupingValue;
		//}

		//// Handle up translation with matching hash-code set as activated (unset the activated grouping hash-code)
		//void UpdateGroupInfoForUp(const std::size_t groupIndex)
		//{
		//	m_exclusivityGroupInfo[groupIndex] = GetGroupInfoForUnsetHashcode(m_exclusivityGroupInfo[groupIndex]);
		//}

		//// Handle up translation with matching hash-code set as activated--but also more than one mapping in the overtaken buffer.
		//void UpdateGroupInfoForUpOfActivatedHash(const std::size_t groupIndex)
		//{
		//	using std::ranges::begin,
		//		std::ranges::cbegin,
		//		std::ranges::end,
		//		std::ranges::cend,
		//		std::ranges::remove;

		//	auto& currentGroupInfo = m_exclusivityGroupInfo[groupIndex];
		//	GroupActivationInfo newGroup{
		//		.GroupingValue = currentGroupInfo.GroupingValue,
		//		.ActivatedMappingHash = {},
		//		.OvertakenHashes = {}
		//	};

		//	auto& currentOvertakenBuffer = currentGroupInfo.OvertakenHashes;
		//	const auto firstElement = cbegin(currentOvertakenBuffer);
		//	if (firstElement != cend(currentOvertakenBuffer))
		//	{
		//		newGroup.ActivatedMappingHash = *firstElement;
		//		currentOvertakenBuffer.erase(firstElement);
		//		newGroup.OvertakenHashes = currentOvertakenBuffer;
		//	}

		//	//auto& currentGroup = m_exclusivityGroupInfo[groupIndex];
		//	//auto tempGroupInfo = GetGroupInfoForUnsetHashcode(currentGroup);
		//	//const bool isOvertakenBufferEmpty = currentGroup.OvertakenHashes.empty();
		//	//if(!isOvertakenBufferEmpty)
		//	//{
		//	//	const auto hashedValueForFirstInLine = *currentGroup.OvertakenHashes.begin();
		//	//	currentGroup.OvertakenHashes.erase(currentGroup.OvertakenHashes.begin());
		//	//	tempGroupInfo.ActivatedMappingHash = hashedValueForFirstInLine;
		//	//}
		//	//tempGroupInfo.OvertakenHashes = m_exclusivityGroupInfo[groupIndex].OvertakenHashes;
		//	//m_exclusivityGroupInfo[groupIndex] = tempGroupInfo;
		//}
	};

}
