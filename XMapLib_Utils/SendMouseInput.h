#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <type_traits>

namespace sds::Utilities
{
	/// <summary>
	/// Utility class for simulating mouse movement input via the Windows API.
	/// </summary>
	class SendMouseInput final
	{
		INPUT m_mouseMoveInput{};
	public:
		/// <summary>Default Constructor</summary>
		SendMouseInput()
		{
			m_mouseMoveInput.type = INPUT_MOUSE;
			m_mouseMoveInput.mi.dwFlags = MOUSEEVENTF_MOVE;
		}
		SendMouseInput(const SendMouseInput& other) = default;
		SendMouseInput(SendMouseInput&& other) = default;
		SendMouseInput& operator=(const SendMouseInput& other) = default;
		SendMouseInput& operator=(SendMouseInput&& other) = default;
		~SendMouseInput() = default;
		/// <summary>Sends mouse movement specified by X and Y number of pixels to move.</summary>
		///	<remarks>Cartesian coordinate plane, starting at 0,0</remarks>
		/// <param name="x">number of pixels in X</param>
		/// <param name="y">number of pixels in Y</param>
		void SendMouseMove(const int x, const int y) noexcept
		{
			using dx_t = decltype(m_mouseMoveInput.mi.dx);
			using dy_t = decltype(m_mouseMoveInput.mi.dy);
			m_mouseMoveInput.mi.dx = static_cast<dx_t>(x);
			m_mouseMoveInput.mi.dy = -static_cast<dy_t>(y);
			m_mouseMoveInput.mi.dwExtraInfo = GetMessageExtraInfo();
			//Finally, send the input
			CallSendInput(&m_mouseMoveInput, 1);
		}
	};

	// Compile-time asserts for the type above, copyable, moveable.
	static_assert(std::is_copy_constructible_v<SendMouseInput>);
	static_assert(std::is_copy_assignable_v<SendMouseInput>);
	static_assert(std::is_move_constructible_v<SendMouseInput>);
	static_assert(std::is_move_assignable_v<SendMouseInput>);

}
