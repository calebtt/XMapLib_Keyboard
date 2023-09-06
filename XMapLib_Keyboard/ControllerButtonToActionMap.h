#pragma once
#include "KeyboardCustomTypes.h"
#include "KeyboardSettingsPack.h"
#include "../DelayManagerProj/DelayManager/DelayManager.hpp"

#include <concepts>

namespace sds
{
	enum class ActionState : int
	{
		INIT, // State indicating ready for new cycle
		KEYDOWN,
		KEYREPEAT,
		KEYUP,
	};

	/**
	 * \brief Wrapper for button to action mapping state enum, the least I can do is make sure state modifications occur through a managing class,
	 * and that there exists only one 'current' state, and that it can only be a finite set of possibilities.
	 * Also contains last sent time (for key-repeat), delay before first key-repeat timer, and a keyboard settings pack.
	 */
	class MappingStateManager final
	{
		ActionState m_currentValue{ ActionState::INIT };
		KeyboardSettings m_keyDefaults{};
	public:
		friend auto hash_value(const MappingStateManager& obj) -> std::size_t
		{
			std::size_t seed = 0x577FCF44;
			seed ^= (seed << 6) + (seed >> 2) + 0x6E3C77E8 + static_cast<std::size_t>(obj.m_currentValue);
			seed ^= (seed << 6) + (seed >> 2) + 0x6FEC6714 + hash_value(obj.m_keyDefaults);
			return seed;
		}
	public:
		/**
		 * \brief	This delay is mostly used for in-between key-repeats, but could also be in between other state transitions.
		 */
		DelayManagement::DelayManager LastSentTime{ m_keyDefaults.KeyRepeatDelay };
		/**
		 * \brief	This is the delay before the first repeat is sent whilst holding the button down.
		 */
		DelayManagement::DelayManager DelayBeforeFirstRepeat{ LastSentTime.GetTimerPeriod() };
	public:
		[[nodiscard]] constexpr bool IsRepeating() const noexcept { return m_currentValue == ActionState::KEYREPEAT; }
		[[nodiscard]] constexpr bool IsDown() const noexcept { return m_currentValue == ActionState::KEYDOWN; }
		[[nodiscard]] constexpr bool IsUp() const noexcept { return m_currentValue == ActionState::KEYUP; }
		[[nodiscard]] constexpr bool IsInitialState() const noexcept { return m_currentValue == ActionState::INIT; }
		constexpr auto SetDown() noexcept { m_currentValue = ActionState::KEYDOWN; }
		constexpr auto SetUp() noexcept { m_currentValue = ActionState::KEYUP; }
		constexpr auto SetRepeat() noexcept { m_currentValue = ActionState::KEYREPEAT; }
		constexpr auto SetInitial() noexcept { m_currentValue = ActionState::INIT; }
	};

	static_assert(std::copyable<MappingStateManager>);
	static_assert(std::movable<MappingStateManager>);

	
	/**
	 * \brief Used to determine if the MappingStateManager is in a state that would require some cleanup before destruction.
	 * \remarks If you add another state for the mapping, make sure to update this.
	 * \return True if mapping needs cleanup, false otherwise.
	 */
	[[nodiscard]]
	constexpr
	bool DoesMappingNeedCleanup(const MappingStateManager& mapping) noexcept
	{
		return mapping.IsDown() || mapping.IsRepeating();
	}

	/**
	 * \brief	Controller button to action mapping. This is how a mapping of a controller button to an action is described.
	 */
	struct CBActionMap final
	{
		/**
		 * \brief	Controller button Virtual Keycode. Can be platform dependent or custom mapping, depends on input poller behavior.
		 */
		keyboardtypes::VirtualKey_t ButtonVirtualKeycode{};

		/**
		 * \brief	If 'true', upon the button being held down, will translate to the key-repeat function activating repeatedly
		 *	using a delay in between repeats.
		 */
		bool UsesInfiniteRepeat{ true };

		/**
		 * \brief	If 'true', upon the button being held down, will send a single repeat, will not continue translating to repeat after the single repeat.
		 * \remarks Note that UsesInfiniteRepeat is expected to be set to 'false' for this to have a meaningful impact.
		 */
		bool SendsFirstRepeatOnly{ false };

		/**
		 * \brief	The exclusivity grouping member is intended to allow the user to add different groups of mappings
		 *	that require another mapping from the same group to be "overtaken" or key-up sent before the "overtaking" new mapping
		 *	can perform the key-down.
		 * \remarks		optional, if not in use set to default constructed value or '{}'
		 */
		keyboardtypes::OptGrp_t ExclusivityGrouping; // TODO one variation of ex. group behavior is to have a priority value associated with the mapping.
		keyboardtypes::Fn_t OnDown; // Key-down
		keyboardtypes::Fn_t OnUp; // Key-up
		keyboardtypes::Fn_t OnRepeat; // Key-repeat
		keyboardtypes::Fn_t OnReset; // Reset after key-up prior to another key-down
		keyboardtypes::OptNanosDelay_t DelayBeforeFirstRepeat; // optional custom delay before first key-repeat
		keyboardtypes::OptNanosDelay_t DelayForRepeats; // optional custom delay between key-repeats
		MappingStateManager LastAction; // Last action performed, with get/set methods.
	public:
		// TODO member funcs for setting delays aren't used because it would screw up the nice braced initialization list mapping construction.
		// TODO maybe should add a real constructor or something? The idea is to encapsulate the data as much as possible.
		// And the OptNanosDelay_t leaves a bit more to be encapsulated.
	};

	static_assert(std::copyable<CBActionMap>);
	static_assert(std::movable<CBActionMap>);

}
