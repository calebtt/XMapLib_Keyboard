#pragma once
#include <SFML/Main.hpp>
#include <SFML/window/Joystick.hpp>

namespace sds
{
	namespace ControllerStatus
	{
		// Returns controller connected status.
		[[nodiscard]]
		inline
		bool IsControllerConnected(const int pid) noexcept
		{
			return sf::Joystick::isConnected(pid);
		}
	}
}
