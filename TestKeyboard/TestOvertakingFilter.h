#pragma once
#include "pch.h"
#include "CppUnitTest.h"
#include "TestMappingProvider.h"
#include "../XMapLib_Keyboard/KeyboardOvertakingFilter.h"

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

			const auto indexA = sds::GetMappingIndexForVk(sds::VirtualButtons::A, mappings);
			const auto indexB = sds::GetMappingIndexForVk(sds::VirtualButtons::B, mappings);
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
			auto filteredState = filt.GetFilteredButtonState({sds::VirtualButtons::A, sds::VirtualButtons::B });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual((int)sds::VirtualButtons::A, (int)filteredState.front());

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			filteredState = filt.GetFilteredButtonState({ sds::VirtualButtons::X, sds::VirtualButtons::B });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual((int)sds::VirtualButtons::X, (int)filteredState.front());

			// now we will remove ButtonX and see that ButtonB has replaced it and needs a key-down.
			filteredState = filt.GetFilteredButtonState({ sds::VirtualButtons::B });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual((int)sds::VirtualButtons::B, (int)filteredState.front());

			// for this case, buttonB is activated, buttonX overtakes it, and buttonY is just a duplicate (with a matching group) that gets filtered.
			filteredState = filt.GetFilteredButtonState({ sds::VirtualButtons::B, sds::VirtualButtons::X, sds::VirtualButtons::Y });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual((int)sds::VirtualButtons::X, (int)filteredState.front());

			// Same as last state, different ordering, and this time it will process the next overtaking.
			filteredState = filt.GetFilteredButtonState({ sds::VirtualButtons::B, sds::VirtualButtons::X, sds::VirtualButtons::Y });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual( (int)sds::VirtualButtons::Y, (int)filteredState.front());
			// Post: ButtonY activated, X and B overtaken.

			filteredState = filt.GetFilteredButtonState({ sds::VirtualButtons::X, sds::VirtualButtons::Y, sds::VirtualButtons::B });
			Assert::AreEqual( 1ull, filteredState.size());
			Assert::AreEqual((int)sds::VirtualButtons::Y, (int)filteredState.front());

			filteredState = filt.GetFilteredButtonState({ sds::VirtualButtons::B, sds::VirtualButtons::X, sds::VirtualButtons::Y, sds::VirtualButtons::A });
			Assert::AreEqual(1ull, filteredState.size());
			Assert::AreEqual((int)sds::VirtualButtons::A, (int)filteredState.front());

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
			auto translationPack = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			// Post: B overtaken, A down.

			Logger::WriteMessage("A and B again, it should filter it so that only ButtonB will be sent a down after A goes up.\n");
			translationPack = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			translationPack();
			Assert::IsTrue(translationPack.UpRequests.size() == 1);
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			// Post: B overtook A, so B is next-state and A is overtaken (key-up)

			// X and B are in the same ex. group, it should filter it so only ButtonX will be sent a down.
			Logger::WriteMessage("X and B are in the same ex. group, it should filter it so that only ButtonX will be sent a down after B goes up.\n");
			translationPack = translator({ sds::VirtualButtons::X, sds::VirtualButtons::B });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			Logger::WriteMessage("X, B, Y, A are in the same ex. group, it should filter it so that only ButtonY will be sent a down after X goes up.\n");
			translationPack = translator({ sds::VirtualButtons::X, sds::VirtualButtons::B, sds::VirtualButtons::Y, sds::VirtualButtons::A });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			Logger::WriteMessage("X, B, Y, A are in the same ex. group, it should filter it so that only ButtonA will be sent a down after Y goes up.\n");
			translationPack = translator({ sds::VirtualButtons::X, sds::VirtualButtons::B, sds::VirtualButtons::Y, sds::VirtualButtons::A });
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
			auto translationPack = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);

			Logger::WriteMessage("B overtakes A, it should filter it so that only ButtonB will be sent a down, A is overtaken.\n");
			translationPack = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			Logger::WriteMessage("Y overtakes B, it should filter it so that only ButtonY will be sent a down, B is overtaken.\n");
			translationPack = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B, sds::VirtualButtons::Y });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.size() == 1);
			Assert::IsTrue(translationPack.UpRequests.size() == 1);

			// Note that multiple keys in the overtaken queue can be removed from the overtaken queue in one iteration, plus the single modification for their group.
			Logger::WriteMessage("A,B removed from overtaken queue, Y still activated (no change to activated key).\n");
			translationPack = translator({ sds::VirtualButtons::Y });
			translationPack();
			Assert::IsTrue(translationPack.DownRequests.empty());
			Assert::IsTrue(translationPack.UpRequests.empty());

			Logger::WriteMessage("A few iterations to set the state for next test...\n");
			// Add buttons A,B back to the overtaken queue with Y activated.
			translator({  })();
			translator({ sds::VirtualButtons::A })();
			translator({ sds::VirtualButtons::A, sds::VirtualButtons::B })();
			translator({ sds::VirtualButtons::A, sds::VirtualButtons::B, sds::VirtualButtons::Y })();

			// Note that multiple keys in the overtaken queue can be removed from the overtaken queue in one iteration, plus the single modification for their group.
			Logger::WriteMessage("With Y activated, A,B overtaken\n");
			Logger::WriteMessage("X overtakes Y, it should filter it so that only X will be sent a down, Y is overtaken. A,B are removed from overtaken queue.\n");
			translationPack = translator({ sds::VirtualButtons::Y, sds::VirtualButtons::X });
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

		TEST_METHOD(FilteredTranslatorStateUpdating)
		{
			using namespace std::chrono_literals;
			using namespace std::chrono;

			auto translator = GetBuiltTranslator();

			// The specific behavior under test here is that only valid, repeatable, state updates will occur, even when dubiously storing multiple translation iterations without intermediate mapping state updates.
			Logger::WriteMessage("First test batch.\n");
			auto translation1 = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			auto translation2 = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			auto translation3 = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			translation1();
			translation2();
			translation3();
			std::this_thread::sleep_for(100ms);
			Logger::WriteMessage("Second test batch.\n");
			auto translation4 = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			auto translation5 = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			auto translation6 = translator({ sds::VirtualButtons::A, sds::VirtualButtons::B });
			translation4();
			translation5();
			translation6();
			translation1();
			translation2();
			translation3();
			Logger::WriteMessage("Third test batch.\n");
			auto translation7 = translator({ });
			auto translation8 = translator({ });
			auto translation9 = translator({ });
			translation7();
			translation8();
			translation9();
			Logger::WriteMessage("Cleanup.\n");
			auto cleanup = translator.GetCleanupActions();
			for (const auto& action : cleanup)
				action();
		}
	};
}