#include <string.h>

#include "cgreen/cgreen.h"
#include "fff.h"
#include "MaxElements.hpp"

using namespace cgreen;

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(vChipSelect, bool);
FAKE_VALUE_FUNC(uint8_t, vSPITransfer, uint8_t);

MAX7456 Max(vChipSelect, vSPITransfer);

Ensure(call_count_is_14)
{
	vSPITransfer_fake.custom_fake = [](uint8_t Data)
	{
	    printf("[STUB] Transfer = %c\n", Data);
	    return Data;
	};

	Max.Initialize();

	printf("ChipSelect_fake.call_count = %d\n", vChipSelect_fake.call_count);
	printf("SPITransfer_fake.call_count = %d\n", vSPITransfer_fake.call_count);

	assert_that(vChipSelect_fake.call_count, is_equal_to(14));
}

TestSuite *our_tests()
{
    TestSuite *suite = create_test_suite();
    add_test(suite, call_count_is_14);
    return suite;
}

int main(int argc, char **argv)
{
    return run_test_suite(our_tests(), create_text_reporter());
}