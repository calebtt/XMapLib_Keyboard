#pragma once
#include <concepts>
#include <source_location>

#include "DelayTimer.h"
#include "KeyboardCustomTypes.h"

namespace sds
{
	enum class ActionState
	{
		Init, // State indicating ready for new cycle
		KeyDown,
		KeyRepeat,
		KeyUp,
	};

	consteval auto ToString(ActionState actionState) -> const char*
	{
		switch (actionState)
		{
		case ActionState::Init:
			return "Init";
		case ActionState::KeyDown:
			return "KeyDown";
		case ActionState::KeyRepeat:
			return "KeyRepeat";
		case ActionState::KeyUp:
			return "KeyUp";
		}
		return "";
	}

	/**
	 * \brief	Wrapper for button to action mapping state enum, the least I can do is make sure state modifications occur through a managing class,
	 *		and that there exists only one 'current' state, and that it can only be a finite set of possibilities.
	 *		Also contains last sent time (for key-repeat), delay before first key-repeat timer, and a keyboard settings pack.
	 * \remarks	This class enforces an invariant that it's state cannot be altered out of sequence.
	 */
	class MappingStateManager final
	{
		/**
		 * \brief Key Repeat Delay is the time delay a button has in-between activations.
		 */
		static constexpr NanosDelay_t DefaultKeyRepeatDelay{ std::chrono::microseconds{100'000} };
		ActionState m_currentValue{ ActionState::Init };
	public:
		/**
		 * \brief	This delay is mostly used for in-between key-repeats, but could also be in between other state transitions.
		 */
		DelayTimer LastSentTime{ DefaultKeyRepeatDelay };
		/**
		 * \brief	This is the delay before the first repeat is sent whilst holding the button down.
		 */
		DelayTimer DelayBeforeFirstRepeat{ LastSentTime.GetTimerPeriod() };
	public:
		[[nodiscard]] constexpr bool IsRepeating() const noexcept { return m_currentValue == ActionState::KeyRepeat; }
		[[nodiscard]] constexpr bool IsDown() const noexcept { return m_currentValue == ActionState::KeyDown; }
		[[nodiscard]] constexpr bool IsUp() const noexcept { return m_currentValue == ActionState::KeyUp; }
		[[nodiscard]] constexpr bool IsInitialState() const noexcept { return m_currentValue == ActionState::Init; }
		constexpr auto SetDown() noexcept
		{
			if (m_currentValue != ActionState::Init)
				return;

			m_currentValue = ActionState::KeyDown;
		}
		constexpr auto SetUp() noexcept
		{
			if (m_currentValue != ActionState::KeyDown && m_currentValue != ActionState::KeyRepeat)
				return;

			m_currentValue = ActionState::KeyUp;
		}
		constexpr auto SetRepeat() noexcept
		{
			if (m_currentValue != ActionState::KeyDown)
				return;

			m_currentValue = ActionState::KeyRepeat;
		}
		constexpr auto SetInitial() noexcept
		{
			if (m_currentValue != ActionState::KeyUp)
				return;

			m_currentValue = ActionState::Init;
		}
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
}
