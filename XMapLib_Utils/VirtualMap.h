#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace sds::Utilities
{
	namespace VirtualMap
	{
		namespace detail
		{
			using ScanCodeType = unsigned short;
			using VirtualKeyType = unsigned int;
			using PrintableType = char;
		}

		/// <summary> Utility function to map a Virtual Keycode to a char </summary>
		///	<returns> printable char value or 0 on error </returns>
		[[nodiscard]]
		inline
		auto GetCharFromVK(const detail::VirtualKeyType vk) noexcept -> detail::PrintableType
		{
			return static_cast<detail::PrintableType>(MapVirtualKeyA(vk, MAPVK_VK_TO_CHAR));
		}

		/// <summary> Utility function to map a char to a Virtual Keycode </summary>
		/// <returns> printable char value or 0 on error </returns>
		[[nodiscard]] 
		inline
		auto GetVKFromChar(const detail::PrintableType letter) noexcept -> detail::VirtualKeyType
		{
			return static_cast<detail::VirtualKeyType>(VkKeyScanA(letter));
		}
	}
}