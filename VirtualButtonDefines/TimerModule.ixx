module;
export module TimerModule;

import std;

export void TimerFunction()
{
	using namespace std::chrono_literals;
	std::this_thread::sleep_for(1s);
}