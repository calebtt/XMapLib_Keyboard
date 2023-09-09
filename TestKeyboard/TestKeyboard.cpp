#include "pch.h"
#include "CppUnitTest.h"
#include "TestMappingProvider.h"
#include "TestOvertakingFilter.h"
#include "TestGroupActivationInfo.h"
#include <filesystem>
#include "../XMapLib_Keyboard/KeyboardOvertakingFilter.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
	TEST_CLASS(TestKeyboard)
	{
        // Test translator with no exclusivity group filter.
		TEST_METHOD(TranslatorTest)
		{
            using namespace std::chrono_literals;
            using namespace std::chrono;

            constexpr sds::keyboardtypes::VirtualKey_t buttonA{ XINPUT_GAMEPAD_A };
            constexpr sds::keyboardtypes::VirtualKey_t buttonB{ XINPUT_GAMEPAD_B };

            auto maps1 = GetMapping(buttonA);
            auto maps2 = GetMapping(buttonB);
            maps2.append_range(maps1);
            sds::KeyboardTranslator poller{ std::move(maps2) };
            const auto translations1 = poller( {buttonA, buttonB} );
            Assert::IsTrue(translations1.DownRequests.size() == 2, L"Translation count not 2.");
            translations1();

            const auto translations2 = poller( {} );
            Assert::IsTrue(translations2.UpRequests.size() == 2, L"Empty state not creating 2 translations after down.");
            translations2();
		}

        TEST_METHOD(TestMovingTranslator)
		{
            auto translator = GetBuiltTranslator();
            auto movedIntoTranslator{ std::move(translator) };

            movedIntoTranslator = GetBuiltTranslator();
		}

        // Short test of filter without translator.
        TEST_METHOD(OvertakerTest)
        {
            using namespace std::chrono_literals;
            using namespace std::chrono;

            constexpr sds::keyboardtypes::VirtualKey_t buttonA{ XINPUT_GAMEPAD_A };
            constexpr sds::keyboardtypes::VirtualKey_t buttonB{ XINPUT_GAMEPAD_B };

            sds::keyboardtypes::SmallVector_t<sds::keyboardtypes::VirtualKey_t> stateUpdate{ buttonA, buttonB };
            sds::keyboardtypes::SmallVector_t<sds::keyboardtypes::VirtualKey_t> emptyState{};

            // Make the filter type.
            sds::KeyboardOvertakingFilter filter;
            // Make the mapping range
            auto maps1 = GetMapping(buttonA, 101);
            auto maps2 = GetMapping(buttonB, 101);
            maps2.append_range(maps1);
            // Set mapping range on the filter (not necessary for normal use aka moved into the translator).
            filter.SetMappingRange(maps2);

            const auto stateUpdatesRange = filter.GetFilteredButtonState({ buttonA, buttonB });
            Assert::IsTrue(stateUpdatesRange.size() == 1, L"State update count not 1.");

            const auto stateUpdatesRange2 = filter.GetFilteredButtonState({});
            Assert::IsTrue(stateUpdatesRange2.empty(), L"Empty state not creating empty state update after both up.");
        }
	};

}
