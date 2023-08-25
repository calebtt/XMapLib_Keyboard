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

			const auto DoDownTestFor = [&](const int n, const bool isFiltered, const bool isKeyUpSent)
			{
				const auto [doFilter, keyUpOpt] = gai.UpdateForNewMatchingGroupingDown(n);
				Assert::IsTrue(doFilter == isFiltered);
				Assert::IsTrue(keyUpOpt.has_value() == isKeyUpSent);
			};
			const auto DoUpTestFor = [&](const int n, const bool isKeyDownSent)
			{
				const auto keyDownOpt = gai.UpdateForNewMatchingGroupingUp(n);
				Assert::IsTrue(keyDownOpt.has_value() == isKeyDownSent);
			};

			// Downs
			DoDownTestFor(1, false, false);
			DoDownTestFor(2, false, true);
			DoDownTestFor(1, true, false);

			// Then ups
			DoUpTestFor(2, true);
			DoUpTestFor(1, false);
			// A false/bad 'up'
			DoUpTestFor(3, false);

			gai = {};
			// Then interleaved downs/ups
			DoDownTestFor(1, false, false);
			DoUpTestFor(2, false);
			DoUpTestFor(1, false);
			DoDownTestFor(2, false, false);
			DoDownTestFor(3, false, true);
			DoDownTestFor(2, true, false);
			DoUpTestFor(2, false);
			DoUpTestFor(3, false);

			// More downs then ups
			DoDownTestFor(1, false, false);
			DoDownTestFor(2, false, true);
			DoDownTestFor(1, true, false);
			DoDownTestFor(3, false, true);

			DoUpTestFor(2, false);
			DoUpTestFor(1, false);
			// A false/bad 'up'
			DoUpTestFor(4, false);
			DoUpTestFor(3, false);
		}
	};
}