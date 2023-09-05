#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <type_traits>

#include "SendKeyInput.h"

namespace sds::Utilities
{
	/// <summary>Sends mouse movement specified by X and Y number of pixels to move.</summary>
	///	<remarks>Cartesian coordinate plane, starting at 0,0</remarks>
	/// <param name="x">number of pixels in X</param>
	/// <param name="y">number of pixels in Y</param>
	inline
	void SendMouseMove(const int x, const int y) noexcept
	{
		INPUT m_mouseMoveInput{};
		m_mouseMoveInput.type = INPUT_MOUSE;
		m_mouseMoveInput.mi.dwFlags = MOUSEEVENTF_MOVE;

		using dx_t = decltype(m_mouseMoveInput.mi.dx);
		using dy_t = decltype(m_mouseMoveInput.mi.dy);
		m_mouseMoveInput.mi.dx = static_cast<dx_t>(x);
		m_mouseMoveInput.mi.dy = -static_cast<dy_t>(y);
		m_mouseMoveInput.mi.dwExtraInfo = GetMessageExtraInfo();
		//Finally, send the input
		CallSendInput(&m_mouseMoveInput, 1);
	}
}
