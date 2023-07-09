#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace sds
{
	namespace ControllerStatus
	{
		// Returns controller connected status.
		[[nodiscard]]
		inline
		bool IsControllerConnected(const int pid) noexcept
		{
			XINPUT_KEYSTROKE keystrokeObj{};
			XINPUT_STATE stateObj{};
			const auto xsRet = XInputGetState(pid, &stateObj);
			const DWORD ret = XInputGetKeystroke(pid, 0, &keystrokeObj);
			return (ret == ERROR_SUCCESS || ret == ERROR_EMPTY) && (xsRet == ERROR_SUCCESS || xsRet == ERROR_EMPTY);
		}
	}
}