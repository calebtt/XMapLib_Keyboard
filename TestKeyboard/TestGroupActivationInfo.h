#pragma once
#include "pch.h"
#include <CppUnitTest.h>
#include "../XMapLib_Keyboard/KeyboardOvertakingFilter.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace TestKeyboard
{
	TEST_CLASS(TestGroupActivationInfo)
	{
		TEST_METHOD(PrimaryTest)
		{
			sds::GroupActivationInfo gai;
			gai.GroupingValue = 101;

			const auto DoDownTestFor = [&](const sds::VirtualButtons n, const bool isFiltered, const bool isKeyUpSent)
			{
				const auto [doFilter, keyUpOpt] = gai.UpdateForNewMatchingGroupingDown(n);
				Assert::IsTrue(doFilter == isFiltered);
				Assert::IsTrue(keyUpOpt.has_value() == isKeyUpSent);
			};
			const auto DoUpTestFor = [&](const sds::VirtualButtons n, const bool isKeyDownSent)
			{
				const auto keyDownOpt = gai.UpdateForNewMatchingGroupingUp(n);
				Assert::IsTrue(keyDownOpt.has_value() == isKeyDownSent);
			};

			const auto One{ sds::VirtualButtons::A };
			const auto Two{ sds::VirtualButtons::X };
			const auto Three{ sds::VirtualButtons::Y };
			const auto Four{ sds::VirtualButtons::ShoulderLeft };

			// Downs
			DoDownTestFor(One, false, false);
			DoDownTestFor(Two, false, true);
			DoDownTestFor(One, true, false);

			// Then ups
			DoUpTestFor(Two, true);
			DoUpTestFor(One, false);
			// A false/bad 'up'
			DoUpTestFor(Three, false);

			gai = {};
			// Then interleaved downs/ups
			DoDownTestFor(One, false, false);
			DoUpTestFor(Two, false);
			DoUpTestFor(One, false);
			DoDownTestFor(Two, false, false);
			DoDownTestFor(Three, false, true);
			DoDownTestFor(Two, true, false);
			DoUpTestFor(Two, false);
			DoUpTestFor(Three, false);

			// More downs then ups
			DoDownTestFor(One, false, false);
			DoDownTestFor(Two, false, true);
			DoDownTestFor(One, true, false);
			DoDownTestFor(Three, false, true);

			DoUpTestFor(Two, false);
			DoUpTestFor(One, false);
			// A false/bad 'up'
			DoUpTestFor(Four, false);
			DoUpTestFor(Three, false);
		}
	};
}