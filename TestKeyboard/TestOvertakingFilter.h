#pragma once
#include "pch.h"
#include "CppUnitTest.h"
#include "TestMappingProvider.h"
#include "TestPollProvider.h"
#include "../XMapLib_Keyboard/KeyboardOvertakingFilter.h"

#include <filesystem>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
	TEST_CLASS(TestOvertakingFilter)
	{

	public:
		TEST_METHOD(TestFreeFuncs)
		{
			auto mappings = GetDriverButtonMappings();

			const auto indexA = sds::GetMappingIndexForVk(ButtonA, mappings);
			const auto indexB = sds::GetMappingIndexForVk(ButtonB, mappings);
			Assert::IsTrue(indexA == 0);
			Assert::IsTrue(indexB == 1);
		}

		TEST_METHOD(TestFilter)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;
			//static constexpr std::size_t IterationsMax{ 1'000'000 };

			auto mappings = GetDriverButtonMappings();
			Assert::IsTrue(mappings.size() > 1);

			sds::KeyboardOvertakingFilter filt;
			filt.SetMappingRange(mappings);

			// Begin clock start
			const auto startTime = steady_clock::now();

			// A and B are in the same ex. group, it should filter it so only ButtonA will be sent a down.
			auto filteredState = filt.GetFilteredButtonState({ ButtonA, ButtonB });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ButtonA, filteredState.front());

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			filteredState = filt.GetFilteredButtonState({ ButtonX, ButtonB });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ButtonX, filteredState.front());

			// now we will remove ButtonX and see that ButtonB has replaced it and needs a key-down.
			filteredState = filt.GetFilteredButtonState({ ButtonB });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ButtonB, filteredState.front());

			// for this case, buttonB is activated, buttonX overtakes it, and buttonY is just a duplicate (with a matching group) that gets filtered.
			filteredState = filt.GetFilteredButtonState({ ButtonB, ButtonX, ButtonY });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ButtonX, filteredState.front());

			// Same as last state, different ordering, and this time it will process the next overtaking.
			filteredState = filt.GetFilteredButtonState({ ButtonB, ButtonX, ButtonY });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual( ButtonY, filteredState.front());
			// Post: ButtonY activated, X and B overtaken.

			filteredState = filt.GetFilteredButtonState({ ButtonX, ButtonY, ButtonB });
			Assert::AreEqual( 1ull, filteredState.size());
			Assert::AreEqual(ButtonY, filteredState.front());

			filteredState = filt.GetFilteredButtonState({ ButtonB, ButtonX, ButtonY, ButtonA });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ButtonA, filteredState.front());


			const auto totalTime = steady_clock::now() - startTime;
			Logger::WriteMessage(std::vformat("Total time: {}\n", std::make_format_args(duration_cast<microseconds>(totalTime))).c_str());
		}
	};
}