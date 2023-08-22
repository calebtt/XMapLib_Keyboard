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
			const auto mappings = GetDriverButtonMappings();

			const auto indexA = sds::GetMappingIndexForVk(ButtonA, mappings);
			const auto indexB = sds::GetMappingIndexForVk(ButtonB, mappings);
			Assert::IsTrue(indexA == 0);
			Assert::IsTrue(indexB == 1);
		}
	};
}