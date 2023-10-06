#pragma once
#include <string>
#include <iostream>
#include <limits>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <SFML/window/Joystick.hpp>


namespace sds
{
	namespace detail
	{
		// Note, these are the virtual keycodes received from SFML with a ps5 controller.
		static constexpr keyboardtypes::VirtualKey_t ButtonX{ 0 };
		static constexpr keyboardtypes::VirtualKey_t ButtonA{ 1 };
		static constexpr keyboardtypes::VirtualKey_t ButtonB{ 2 };
		static constexpr keyboardtypes::VirtualKey_t ButtonY{ 3 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeft{ 4 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderRight{ 5 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderLeftLower{ 6 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShoulderRightLower{ 7 };
		static constexpr keyboardtypes::VirtualKey_t ButtonLeftPill{ 8 };
		static constexpr keyboardtypes::VirtualKey_t ButtonRightPill{ 9 };
		static constexpr keyboardtypes::VirtualKey_t ButtonLeftStickClick{ 10 };
		static constexpr keyboardtypes::VirtualKey_t ButtonRightStickClick{ 11 };
		static constexpr keyboardtypes::VirtualKey_t ButtonPlayLogo{ 12 };
		static constexpr keyboardtypes::VirtualKey_t ButtonShiftSwitch{ 13 };

		static constexpr std::array<keyboardtypes::VirtualKey_t, 14> ButtonCodeArray
		{
			ButtonX,
			ButtonA,
			ButtonB,
			ButtonY,
			ButtonShoulderLeft,
			ButtonShoulderRight,
			ButtonShoulderLeftLower,
			ButtonShoulderRightLower,
			ButtonLeftPill,
			ButtonRightPill,
			ButtonLeftStickClick,
			ButtonRightStickClick,
			ButtonPlayLogo,
			ButtonShiftSwitch
		};
	}

	/**
	 * \brief Uses SFML to call some OS API function(s) to retrieve a controller state update.
	 * \param playerId Most commonly 0 for a single device connected.
	 * \return Platform/API specific state structure.
	 */
	[[nodiscard]]
	inline
	auto GetSFMLApiStateUpdate(const int playerId = 0) noexcept -> uint32_t
	{
		using Stick = sf::Joystick;
		using std::numeric_limits;
		using std::cout;
		using std::cin;
		using std::getline;
		using std::string;

		Stick::update();

		if (Stick::isConnected(playerId))
		{
			const auto buttonCount = Stick::getButtonCount(playerId);
			for (uint32_t i{}; i < buttonCount; ++i)
			{
				if (Stick::isButtonPressed(playerId, i))
				{
					cout << "Pressed: " << i << '\n';
				}
			}
			return 1;
		}
		return {};
	}
}

