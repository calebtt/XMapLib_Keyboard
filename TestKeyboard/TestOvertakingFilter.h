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

		// Here we build a large queue of activated/overtaken keys and then key-up them all at once.
		TEST_METHOD(TestLargeQueueToAllUp)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;

			auto translator = GetBuiltTranslator();

			// A and B are in the same ex. group, it should filter it so only ButtonA will be sent a down.
			Logger::WriteMessage("A and B are in the same ex. group, it should filter it so that only ButtonA will be sent a down.\n");
			auto translationPack = translator({ ButtonA, ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 1);
			// Post: B overtaken, A down.

			Logger::WriteMessage("A and B again, it should filter it so that only ButtonB will be sent a down after A goes up.\n");
			translationPack = translator({ ButtonA, ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 2);
			// Post: B overtook A, so B is next-state and A is overtaken (key-up)

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			Logger::WriteMessage("X and B are in the same ex. group, it should filter it so that only ButtonX will be sent a down after B goes up.\n");
			translationPack = translator({ ButtonX, ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 2);

			Logger::WriteMessage("X, B, Y, A are in the same ex. group, it should filter it so that only ButtonY will be sent a down after X goes up.\n");
			translationPack = translator({ ButtonX, ButtonB, ButtonY, ButtonA });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 2);

			Logger::WriteMessage("X, B, Y, A are in the same ex. group, it should filter it so that only ButtonA will be sent a down after Y goes up.\n");
			translationPack = translator({ ButtonX, ButtonB, ButtonY, ButtonA });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 2);


		}

		TEST_METHOD(TestFilterWithTranslator)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;

			auto translator = GetBuiltTranslator();

			// A and B are in the same ex. group, it should filter it so only ButtonA will be sent a down.
			Logger::WriteMessage("A and B are in the same ex. group, it should filter it so that only ButtonA will be sent a down.\n");
			auto translationPack = translator({ ButtonA, ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 1);
			// Post: B overtaken, A down.

			Logger::WriteMessage("A and B again, it should filter it so that only ButtonB will be sent a down after A goes up.\n");
			translationPack = translator({ ButtonA, ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 2);
			// Post: B overtook A, so B is next-state and A is overtaken (key-up)

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			Logger::WriteMessage("X and B are in the same ex. group, it should filter it so that only ButtonX will be sent a down after B goes up.\n");
			translationPack = translator({ ButtonX, ButtonB });
			translationPack();
			Assert::IsTrue(translationPack.NextStateRequests.size() == 2);

			// now we will remove ButtonX and see that ButtonB has replaced it and needs a key-down.
			// TODO I think it's counting the absence of ButtonX as a state update for the ex. group and filtering out the ButtonB.
			Logger::WriteMessage("It should filter it so that only ButtonB will be sent a down after X goes up.\n");
			translationPack = translator({ ButtonB });
			translationPack();
			Assert::IsTrue( translationPack.NextStateRequests.size() == 2);

			//// for this case, buttonB is activated, buttonX overtakes it, and buttonY is just a duplicate (with a matching group) that gets filtered.
			//translationPack = translator({ ButtonB, ButtonX, ButtonY });
			//Assert::AreEqual(1ull, translationPack.size());
			//Assert::AreEqual(ButtonX, translationPack.front());

			//// Same as last state, different ordering, and this time it will process the next overtaking.
			//translationPack = translator({ ButtonB, ButtonX, ButtonY });
			//Assert::AreEqual(1ull, translationPack.size());
			//Assert::AreEqual(ButtonY, translationPack.front());
			//// Post: ButtonY activated, X and B overtaken.

			//translationPack = translator({ ButtonX, ButtonY, ButtonB });
			//Assert::AreEqual(1ull, translationPack.size());
			//Assert::AreEqual(ButtonY, translationPack.front());

			//translationPack = translator({ ButtonB, ButtonX, ButtonY, ButtonA });
			//Assert::AreEqual(1ull, translationPack.size());
			//Assert::AreEqual(ButtonA, translationPack.front());
		}
	};
}