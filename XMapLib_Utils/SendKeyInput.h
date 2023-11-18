#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <type_traits>
#include <bitset>
#include <climits>
#include <optional>
#include <future>

#include <SFML/Main.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

namespace sds::Utilities
{
	/**
	 * \brief	Utility function to map a Virtual Keycode to a scancode
	 * \param	vk integer virtual keycode
	 * \return	returns the hardware scancode of the key
	 */
	[[nodiscard]]
	inline
	auto GetScanCode(const int vk) noexcept -> std::optional<WORD>
	{
		using VirtKey_t = unsigned char;
		if (vk > std::numeric_limits<VirtKey_t>::max()
			|| vk < std::numeric_limits<VirtKey_t>::min())
			return {};
		const auto scan = static_cast<WORD> (MapVirtualKeyExA(vk, MAPVK_VK_TO_VSC, nullptr));
		return scan;
	}

	/**
	 * \brief	One function calls SendInput with the eventual built INPUT struct.
	 *	This is useful for debugging or re-routing the output for logging/testing of a near-real-time system.
	 * \param inp	Pointer to first element of INPUT array.
	 * \param numSent	Number of elements in the array to send.
	 * \return	Returns number of input structures sent.
	 */
	inline
	auto CallSendInput(INPUT* inp, std::uint32_t numSent) noexcept -> UINT
	{
		return SendInput(static_cast<UINT>(numSent), inp, sizeof(INPUT));
	}

	/**
	 * \brief	Utility function to send a virtual keycode as input to the OS.
	 * \param vk	virtual keycode for key (not always the same as a hardware scan code!)
	 * \param isKeyboard	Is the source a keyboard or mouse?
	 * \param sendDown	Send key-down event?
	 * \return	Returns number of events sent.
	 * \remarks		Handles keyboard keys and several mouse click buttons.
	 */
	inline
	UINT SendVirtualKey(const auto vk, const bool isKeyboard, const bool sendDown) noexcept
	{
		INPUT inp{};
		inp.type = isKeyboard ? INPUT_KEYBOARD : INPUT_MOUSE;
		if (isKeyboard)
		{
			inp.ki.dwFlags = sendDown ? 0 : KEYEVENTF_KEYUP;
			inp.ki.wVk = static_cast<WORD>(vk);
			inp.ki.dwExtraInfo = GetMessageExtraInfo();
		}
		else
		{
			switch (vk)
			{
			case VK_LBUTTON:
				inp.mi.dwFlags = sendDown ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
				break;
			case VK_RBUTTON:
				inp.mi.dwFlags = sendDown ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
				break;
			case VK_MBUTTON:
				inp.mi.dwFlags = sendDown ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
				break;
			case VK_XBUTTON1:
				[[fallthrough]]; //annotated fallthrough
			case VK_XBUTTON2:
				inp.mi.dwFlags = sendDown ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
				break;
			default:
				return 0;
			}
			inp.mi.dwExtraInfo = GetMessageExtraInfo();
		}
		return CallSendInput(&inp, 1);
	}

	/**
	 * \brief	Sends the virtual keycode as a hardware scancode
	 * \param virtualKeycode	is the Virtual Keycode of the keystroke you wish to emulate
	 * \param doKeyDown		is a boolean denoting if the keypress event is KEYDOWN or KEYUP
	 */
	inline
	bool SendScanCode(const int virtualKeycode, const bool doKeyDown) noexcept
	{
		// Build INPUT struct
		INPUT tempInput = {};
		auto MakeItMouse = [&](const DWORD flagsDown, const DWORD flagsUp, const bool isDown)
		{
			tempInput.type = INPUT_MOUSE;
			if (isDown)
				tempInput.mi.dwFlags = flagsDown;
			else
				tempInput.mi.dwFlags = flagsUp;
			tempInput.mi.dwExtraInfo = GetMessageExtraInfo();
			return CallSendInput(&tempInput, 1);
		};

		const auto scanCode = GetScanCode(virtualKeycode);
		if (!scanCode)
		{
			//try mouse
			switch (virtualKeycode)
			{
			case VK_LBUTTON:
				return MakeItMouse(MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP, doKeyDown) != 0;
			case VK_RBUTTON:
				return MakeItMouse(MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP, doKeyDown) != 0;
			case VK_MBUTTON:
				return MakeItMouse(MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP, doKeyDown) != 0;
			case VK_XBUTTON1:
				[[fallthrough]];
			case VK_XBUTTON2:
				return MakeItMouse(MOUSEEVENTF_XDOWN, MOUSEEVENTF_XUP, doKeyDown) != 0;
			default:
				break;
			}
			return false;
		}

		//do scancode
		tempInput = {};
		tempInput.type = INPUT_KEYBOARD;
		tempInput.ki.dwFlags = doKeyDown ? tempInput.ki.dwFlags = KEYEVENTF_SCANCODE
			: tempInput.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
		tempInput.ki.wScan = scanCode.value();

		const UINT ret = CallSendInput(&tempInput, 1);
		return ret != 0;
	}

	/**
	 * \brief	Function called to un-set "num lock" key asynchronously. It is a "fire and forget" operation
	 *	that spawns a thread and returns a shared_future to it. The thread attempts to unset the key, and will not re-attempt if it fails.
	 * \returns		The shared_future may contain an optional string error message.
	 * \remarks		Uses async()
	 */
	inline
	auto UnsetNumlockAsync() noexcept -> std::optional<std::shared_future<bool>>
	{
		// Endian-ness of machine it's being compiled on.
		static constexpr bool IsLittleEnd{ std::endian::native == std::endian::little };

		const auto NumLockState = GetKeyState(static_cast<int>(VK_NUMLOCK));
		const std::bitset<sizeof(NumLockState)* CHAR_BIT> bits(NumLockState);
		static_assert(bits.size() > 0);
		if (const bool IsNumLockSet = IsLittleEnd ? bits[0] : bits[bits.size() - 1])
		{
			auto DoNumlockSend = [&]() -> bool
			{
				auto result = SendVirtualKey(VK_NUMLOCK, true, true);
				if (result != 1)
					return false;
				std::this_thread::sleep_for(std::chrono::milliseconds(15));
				result = SendVirtualKey(VK_NUMLOCK, true, false);
				if (result != 1)
					return false;
				return true;
			};
			std::shared_future<bool> fut = std::async(std::launch::async, DoNumlockSend);
			return fut;
		}
		return {};
	}

	/**
	 * \brief	Calls UnsetNumlockAsync() and waits for <b>timeout</b> time for a completion result, if timeout then returns nothing.
	 *	If an error occurs within the timeout period, it logs the error.
	 * \param timeoutTime	Time in milliseconds to wait for a result from the asynchronous thread spawned to unset numlock.
	 * \remarks		Do not call this function in a loop, it has a synchronous wait time delay.
	 */
	inline
	bool UnsetAndCheckNumlock(const std::chrono::milliseconds timeoutTime = std::chrono::milliseconds{20})
	{
		// Calls the unset numlock async function, gets a shared_future from which to determine if it succeeded, within a reasonable timeout.
		const auto optFuture = UnsetNumlockAsync();
		// If the future returned indicates an action was performed (numlock was set, it spawned a thread)
		if (optFuture.has_value())
		{
			// Then we wait for completion for the timeout duration
			const auto waitResult = optFuture.value().wait_for(timeoutTime);
			// If we have a result in a reasonable timeframe...
			if (waitResult == std::future_status::ready)
			{
				return optFuture.value().get();
			}
		}
		return true;
	}

}