module;
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Windows.h" // For SendInput

export module MainModule;

import <iostream>;
import <chrono>;
import <thread>;
import <format>;
import <print>;
import <future>;
import <string>;
import <vector>;
import <concepts>;
import <ranges>;
import <functional>;
import <algorithm>;
import <optional>;
import <map>;
import <variant>;
import <memory>;
import <type_traits>;
import <string_view>;
import <cassert>;

import XMapLibBase;
import TranslateAction;
import PluginSfmlPoller;
import OvertakingFilter;

// Crude mechanism to keep the loop running until [enter] is pressed.
struct GetterExitCallable final
{
	std::atomic<bool> IsDone{ false };
	void GetExitSignal()
	{
		std::string buf;
		std::getline(std::cin, buf);
		IsDone.store(true, std::memory_order_relaxed);
	}
};

/// <summary>Sends mouse movement specified by X and Y number of pixels to move.</summary>
///	<remarks>Cartesian coordinate plane, starting at 0,0</remarks>
/// <param name="x">number of pixels in X</param>
/// <param name="y">number of pixels in Y</param>
void SendMouseMove(const int x, const int y) noexcept;

/// <summary>Gets the KeyStateBehaviors pack for a given key name.</summary>
auto GetPrintBehaviorsForKey(const std::string keyName) -> sds::KeyStateBehaviors;

/// <summary>Gets the KeyStateBehaviors pack for thumbstick direction.</summary>
auto GetMouseMoveBehaviors(const sds::ThumbstickDirection stickDir) -> sds::KeyStateBehaviors;

/// <summary>Gets the mappings for the test driver.</summary>
auto GetTestDriverMappings() -> std::vector<sds::MappingContainer>;

export int main()
{
	using namespace sds;
	using std::cout, std::println;
	using namespace std::chrono_literals;

	constexpr int PlayerId{ 0 };
	constexpr float ThumbstickDeadzone{ 30.0f };
	println("This is a C++ modules main.");

	Translator translator{ GetTestDriverMappings() };
	OvertakingFilter<> filter{ translator };

	// Crude mechanism to keep the loop running until [enter] is pressed.
	GetterExitCallable gec;
	const auto exitFuture = std::async(std::launch::async, [&gec]() { gec.GetExitSignal(); });
	while(!gec.IsDone)
	{
		auto translationResult = translator(filter(GetWrappedControllerStateUpdatePs5(PlayerId, ThumbstickDeadzone, ThumbstickDeadzone)));
		translationResult();
	}

	const auto cleanupActions = translator.GetCleanupActions();
	for (const auto& action : cleanupActions)
	{
		action();
	}
}


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
	const auto sendResult = SendInput(1, &m_mouseMoveInput, sizeof(INPUT));
	assert(sendResult == 1);
}

auto GetPrintBehaviorsForKey(const std::string keyName) -> sds::KeyStateBehaviors
{
	using std::cout;
	sds::KeyStateBehaviors behaviors
	{
		.OnDown = [=]() { cout << "[" << keyName << "]-[OnDown]\n"; },
		.OnUp = [=]() { cout << "[" << keyName << "]-[OnUp]\n"; },
		.OnRepeat = [=]() { cout << "[" << keyName << "]-[OnRepeat]\n"; },
		.OnReset = [=]() { cout << "[" << keyName << "]-[OnReset]\n"; }
	};
	return behaviors;
}

auto GetMouseMoveBehaviors(const sds::ThumbstickDirection stickDir) -> sds::KeyStateBehaviors
{
	using std::cout;
	const auto getMovementFn = [&](sds::ThumbstickDirection dir) -> std::function<void()>
	{
		switch (dir)
		{
		case sds::ThumbstickDirection::Up:
			return []() { SendMouseMove(0, 1); };
		case sds::ThumbstickDirection::UpRight:
			return []() { SendMouseMove(1, 1); };
		case sds::ThumbstickDirection::Right:
			return []() { SendMouseMove(1, 0); };
		case sds::ThumbstickDirection::RightDown:
			return []() { SendMouseMove(1, -1); };
		case sds::ThumbstickDirection::Down:
			return []() { SendMouseMove(0, -1); };
		case sds::ThumbstickDirection::DownLeft:
			return []() { SendMouseMove(-1, -1); };
		case sds::ThumbstickDirection::Left:
			return []() { SendMouseMove(-1, 0); };
		case sds::ThumbstickDirection::LeftUp:
			return []() { SendMouseMove(-1, 1); };
		default:
			return []() {};
		}
	};

	sds::KeyStateBehaviors behaviors
	{
		.OnDown = getMovementFn(stickDir),
		.OnRepeat = getMovementFn(stickDir),
	};
	return behaviors;
}

auto GetTestDriverMappings() -> std::vector<sds::MappingContainer>
{
	using namespace sds;
	using std::cout, std::println;
	using namespace std::chrono_literals;
	constexpr int ThumbstickGrouping{ 101 };

	std::vector<MappingContainer> mappings
	{
		MappingContainer
		{
			ButtonDescription{VirtualButtons::Square, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetPrintBehaviorsForKey("Square")}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::Circle, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetPrintBehaviorsForKey("Circle")}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::X, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetPrintBehaviorsForKey("Cross")}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::Triangle, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetPrintBehaviorsForKey("Triangle")}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::ShoulderRight}, 
			KeyStateBehaviors{[]() { system("cls"); }}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickDownLeft, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::DownLeft)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickDown, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::Down)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickDownRight, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::RightDown)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickRight, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::Right)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickUpRight, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::UpRight)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickUp, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::Up)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickUpLeft, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::LeftUp)}, {1ms}, {1ms}
		},
		MappingContainer
		{
			ButtonDescription{VirtualButtons::RightThumbstickLeft, {}, ThumbstickGrouping},
			KeyStateBehaviors{GetMouseMoveBehaviors(ThumbstickDirection::Left)}, {1ms}, {1ms}
		},
	};
	return mappings;
}