#pragma once
#include "pch.h"
#include "CppUnitTest.h"
#include "TestMappingProvider.h"
#include "TestPollProvider.h"
#include "../XMapLib_Keyboard/KeyboardOvertakingFilter.h"

#include <filesystem>

#include "../XMapLib_Keyboard/KeyboardMappingBuilders.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
	[[nodiscard]] inline auto GetBuiltTranslator()
	{
		auto mappings = GetDriverButtonMappings();
		Assert::IsFalse(mappings.empty(), L"Test mappings buffer was created empty!");
		sds::KeyboardOvertakingFilter filt{}; // <-- any complex construction/initialization done here.
		sds::KeyboardTranslator<decltype(filt)> translator{ std::move(mappings), std::move(filt) };
		return translator;
	}

	TEST_CLASS(TestOvertakingFilter)
	{
		static constexpr sds::KeyboardSettings ksp;
	public:
		TEST_METHOD(TestFreeFuncs)
		{
			auto mappings = GetDriverButtonMappings();

			const auto indexA = sds::GetMappingIndexForVk(ksp.ButtonA, mappings);
			const auto indexB = sds::GetMappingIndexForVk(ksp.ButtonB, mappings);
			Assert::IsTrue(indexA == 0);
			Assert::IsTrue(indexB == 1);
		}

		TEST_METHOD(TestFilter)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;

			auto mappings = GetDriverButtonMappings();
			Assert::IsTrue(mappings.size() > 1);

			sds::KeyboardOvertakingFilter filt;
			filt.SetMappingRange(mappings);

			// Begin clock start
			const auto startTime = steady_clock::now();

			// A and B are in the same ex. group, it should filter it so only ButtonA will be sent a down.
			auto filteredState = filt.GetFilteredButtonState({ ksp.ButtonA, ksp.ButtonB });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ksp.ButtonA, filteredState.front());

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			filteredState = filt.GetFilteredButtonState({ ksp.ButtonX, ksp.ButtonB });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ksp.ButtonX, filteredState.front());

			// now we will remove ButtonX and see that ButtonB has replaced it and needs a key-down.
			filteredState = filt.GetFilteredButtonState({ ksp.ButtonB });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ksp.ButtonB, filteredState.front());

			// for this case, buttonB is activated, buttonX overtakes it, and buttonY is just a duplicate (with a matching group) that gets filtered.
			filteredState = filt.GetFilteredButtonState({ ksp.ButtonB, ksp.ButtonX, ksp.ButtonY });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ksp.ButtonX, filteredState.front());

			// Same as last state, different ordering, and this time it will process the next overtaking.
			filteredState = filt.GetFilteredButtonState({ ksp.ButtonB, ksp.ButtonX, ksp.ButtonY });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual( ksp.ButtonY, filteredState.front());
			// Post: ButtonY activated, X and B overtaken.

			filteredState = filt.GetFilteredButtonState({ ksp.ButtonX, ksp.ButtonY, ksp.ButtonB });
			Assert::AreEqual( 1ull, filteredState.size());
			Assert::AreEqual(ksp.ButtonY, filteredState.front());

			filteredState = filt.GetFilteredButtonState({ ksp.ButtonB, ksp.ButtonX, ksp.ButtonY, ksp.ButtonA });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual(ksp.ButtonA, filteredState.front());


			const auto totalTime = steady_clock::now() - startTime;
			Logger::WriteMessage(std::vformat("Total time: {}\n", std::make_format_args(duration_cast<microseconds>(totalTime))).c_str());
		}

		// Here we build a large queue of activated/overtaken keys and then key-up them all at once.
		TEST_METHOD(TestLargeQueueToAllUp)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;

			auto translator = GetBuiltTranslator();

			// A and B are in the same ex. group, it should filter it so only ksp.ButtonA will be sent a down.
			Logger::WriteMessage("A and B are in the same ex. group, it should filter it so that only ButtonA will be sent a down.\n");
			auto translationPack = translator({ ksp.ButtonA, ksp.ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			// Post: B overtaken, A down.

			Logger::WriteMessage("A and B again, it should filter it so that only ButtonB will be sent a down after A goes up.\n");
			translationPack = translator({ ksp.ButtonA, ksp.ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.UpRequests.size() == 1);
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			// Post: B overtook A, so B is next-state and A is overtaken (key-up)

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			Logger::WriteMessage("X and B are in the same ex. group, it should filter it so that only ButtonX will be sent a down after B goes up.\n");
			translationPack = translator({ ksp.ButtonX, ksp.ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			Logger::WriteMessage("X, B, Y, A are in the same ex. group, it should filter it so that only ButtonY will be sent a down after X goes up.\n");
			translationPack = translator({ ksp.ButtonX, ksp.ButtonB, ksp.ButtonY, ksp.ButtonA });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			Logger::WriteMessage("X, B, Y, A are in the same ex. group, it should filter it so that only ButtonA will be sent a down after Y goes up.\n");
			translationPack = translator({ ksp.ButtonX, ksp.ButtonB, ksp.ButtonY, ksp.ButtonA });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);


		}

		TEST_METHOD(TestFilterWithTranslator)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;

			auto translator = GetBuiltTranslator();

			Logger::WriteMessage("A and B are in the same ex. group, it should filter it so that only ButtonA will be sent a down.\n");
			auto translationPack = translator({ ksp.ButtonA, ksp.ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);

			Logger::WriteMessage("B overtakes A, it should filter it so that only ButtonB will be sent a down, A is overtaken.\n");
			translationPack = translator({ ksp.ButtonA, ksp.ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			Logger::WriteMessage("Y overtakes B, it should filter it so that only ButtonY will be sent a down, B is overtaken.\n");
			translationPack = translator({ ksp.ButtonA, ksp.ButtonB, ksp.ButtonY });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			// Note that multiple keys in the overtaken queue can be removed from the overtaken queue in one iteration, plus the single modification for their group.
			Logger::WriteMessage("A,B removed from overtaken queue, Y still activated (no change to activated key).\n");
			translationPack = translator({ ksp.ButtonY });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.empty());
			Assert::IsTrue(translationPack.UpRequests.empty());

			Logger::WriteMessage("A few iterations to set the state for next test...\n");
			// Add buttons A,B back to the overtaken queue with Y activated.
			translator({  })();
			translator({ ksp.ButtonA })();
			translator({ ksp.ButtonA, ksp.ButtonB })();
			translator({ ksp.ButtonA, ksp.ButtonB, ksp.ButtonY })();

			// Note that multiple keys in the overtaken queue can be removed from the overtaken queue in one iteration, plus the single modification for their group.
			Logger::WriteMessage("With Y activated, A,B overtaken\n");
			Logger::WriteMessage("X overtakes Y, it should filter it so that only X will be sent a down, Y is overtaken. A,B are removed from overtaken queue.\n");
			translationPack = translator({ ksp.ButtonY, ksp.ButtonX });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);
		}

		TEST_METHOD(TestCopyAndMovingTheFilter)
		{
			sds::KeyboardOvertakingFilter filt;
			auto mappings = GetDriverButtonMappings();
			filt.SetMappingRange(mappings);

			sds::KeyboardOvertakingFilter secondFilt;
			auto secondMappings = GetDriverButtonMappings();
			secondFilt.SetMappingRange(secondMappings);

			// copy
			filt = secondFilt;

			// move-assign
			filt = std::move(secondFilt);

			// move
			auto newFilt = std::move(filt);

		}
	};
}