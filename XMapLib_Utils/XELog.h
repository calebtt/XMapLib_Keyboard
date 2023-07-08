#pragma once
#include <iostream>
#include <syncstream>

namespace sds::Utilities
{
	enum class XELogLevel
	{
		TRACE,
		DEBUG,
		INFORMATION,
		WARNING,
		ERR,
		CRITICAL
	};

	/// <summary> Error logging function, thread safe. </summary>
	//[[msvc::noinline]]
	inline
	void LogError(const std::string& msg, [[maybe_unused]] XELogLevel level = XELogLevel::DEBUG) noexcept
	{
		if (!msg.empty())
		{
			//osyncstream can be used with concurrency to avoid garbled output,
			//as long as each thread has it's own osyncstream object.
			std::osyncstream syncedOut(std::cerr);
			syncedOut << msg << std::endl;
			syncedOut.emit();
		}
	}
}
