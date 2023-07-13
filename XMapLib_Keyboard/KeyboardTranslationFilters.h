#pragma once
#include "KeyboardLibIncludes.h"
#include "ControllerButtonToActionMap.h"
#include "KeyboardTranslationHelpers.h"

namespace sds
{
	// TODO, may be used to filter the output to apply the overtaking behavior.
	class OvertakingFilter final
	{
		// Logically each exclusivity grouping looks like this.
		struct GroupActivationInfo final
		{
			// Exclusivity grouping value, mirroring the mapping value used.
			keyboardtypes::GrpVal_t GroupingValue{};
			// A value of 0 indicates no mapping is activated.
			std::size_t ActivatedMappingHash{};
		};
		// New translation pack type with an overtaken member.
		struct FilteredTranslationPack final
		{
			explicit FilteredTranslationPack(TranslationPack&& pack)
			{
				UpdateRequests = std::move(pack.UpdateRequests);
				RepeatRequests = std::move(pack.RepeatRequests);
				NextStateRequests = std::move(pack.NextStateRequests);
			}
			void operator()() const
			{
				// Note that there will be a function called if there is a state change,
				// it just may not have any custom behavior attached to it.
				for (const auto& elem : UpdateRequests)
					elem();
				for (const auto& elem : OvertakenRequests)
					elem();
				for (const auto& elem : RepeatRequests)
					elem();
				for (const auto& elem : NextStateRequests)
					elem();
			}
			keyboardtypes::SmallVector_t<TranslationResult> UpdateRequests{};
			keyboardtypes::SmallVector_t<TranslationResult> RepeatRequests{};
			keyboardtypes::SmallVector_t<TranslationResult> OvertakenRequests{};
			keyboardtypes::SmallVector_t<TranslationResult> NextStateRequests{};
		};
	private:
		// Constructed from the mapping list, pairs with ex. group data.
		keyboardtypes::SmallVector_t<GroupActivationInfo> m_exclusivityGroupInfo;
	public:
		explicit OvertakingFilter(std::span<const CBActionMap> mappingsList)
		{
			// Build the buffer of ex. group information.
			for (const auto& elem : mappingsList)
			{
				if (elem.ExclusivityGrouping)
				{
					m_exclusivityGroupInfo.emplace_back(
						GroupActivationInfo
						{
						*elem.ExclusivityGrouping,
						0
						});
				}
			}
		}
	public:
		// If you filter the translationpack, you lose access to the original. Move it into the filter.
		auto operator()(TranslationPack&& translation) -> FilteredTranslationPack
		{
			return FilterTranslationPack(std::move(translation));
		}

		auto FilterTranslationPack(TranslationPack&& translation) -> FilteredTranslationPack
		{
			using std::ranges::find, std::ranges::end, std::ranges::find_if;

			FilteredTranslationPack filteredPack{ std::move(translation) };
			for (auto& nextStateTranslation : filteredPack.NextStateRequests)
			{
				// If it has a group value
				if (nextStateTranslation.ExclusivityGrouping)
				{
					// Find the group info data for it
					const auto groupValue = *nextStateTranslation.ExclusivityGrouping;
					const auto foundItem = find_if(m_exclusivityGroupInfo, [groupValue](const auto& e)
						{
							return e.GroupingValue == groupValue && e.ActivatedMappingHash == 0;
						});
					// If the group info data is not down, set to down.
					const auto mappingHash = nextStateTranslation.MappingHash;
					foundItem->ActivatedMappingHash = mappingHash;
				}
			}
			return filteredPack;
		}
	};
}