export module MappingRangeAlgos;

import <span>;
import <ranges>;
import <stdexcept>;
import <format>;
import <source_location>;
import <algorithm>;
import <functional>;
import <optional>;

import CustomTypes;
import VirtualController;
import ButtonMapping;

export namespace sds
{
	/**
	* \brief Returns the indices at which a mapping that matches the 'vk' was found.
	* \param vk Virtual keycode of the presumably 'down' key with which to match CBActionMap mappings.
	* \param mappingsRange The range of CBActionMap mappings for which to return the indices of matching mappings.
	* \remarks PRECONDITION: A mapping with the specified VK does exist in the mappingsRange!
	*/
	[[nodiscard]] inline auto GetMappingIndexForVk(const VirtualButtons vk, const std::span<const MappingContainer> mappingsRange) -> std::optional<Index_t>
	{
		using std::ranges::find_if;
		using std::ranges::cend;
		using std::ranges::cbegin;
		using std::ranges::distance;

		const auto findResult = find_if(mappingsRange,[vk](const auto e) { return e.Button.ButtonVirtualKeycode == vk; });
		const bool didFindResult = findResult != cend(mappingsRange);

		if (!didFindResult)
		{
			throw std::runtime_error(
				std::vformat("Did not find mapping with vk: {} in mappings range.\nLocation:\n{}\n\n",
					std::make_format_args(static_cast<int>(vk), std::source_location::current().function_name())));
		}

		return static_cast<Index_t>(distance(cbegin(mappingsRange), findResult));
	}

	/**
	* \brief Returns the indices at which a mapping that matches the 'vk' was found.
	* \param vk Virtual keycode of the presumably 'down' key with which to match CBActionMap mappings.
	* \param mappingsRange The range of CBActionMap mappings for which to return the indices of matching mappings.
	*/
	[[nodiscard]] inline auto GetMappingByVk(const VirtualButtons vk, std::span<const MappingContainer> mappingsRange) -> std::optional<std::span<const MappingContainer>::iterator>
	{
		using std::ranges::find_if;
		using std::ranges::cend;
		using std::ranges::cbegin;
		using std::ranges::distance;

		const auto findResult = find_if(mappingsRange, [vk](const auto e) { return e.Button.ButtonVirtualKeycode == vk; });
		const bool didFindResult = findResult != cend(mappingsRange);

		if (!didFindResult)
		{
			return {};
		}

		return findResult;
	}

	[[nodiscard]] constexpr auto IsVkInStateUpdate(const auto vkToFind, const std::span<const VirtualButtons> downVirtualKeys) noexcept -> bool
	{
		return std::ranges::any_of(downVirtualKeys, [vkToFind](const auto vk) { return vk == vkToFind; });
	}

	[[nodiscard]] constexpr auto IsMappingInRange(const VirtualButtons vkToFind, const std::ranges::range auto& downVirtualKeys) noexcept -> bool
	{
		return std::ranges::any_of(downVirtualKeys, [vkToFind](const auto vk) { return vk == vkToFind; });
	}

	constexpr void EraseValuesFromRange(std::ranges::range auto& theRange, const std::ranges::range auto& theValues) noexcept
	{
		for (const auto& elem : theValues)
		{
			const auto foundPosition = std::ranges::find(theRange, elem);
			if (foundPosition != std::ranges::cend(theRange))
				theRange.erase(foundPosition);
		}
	}

}