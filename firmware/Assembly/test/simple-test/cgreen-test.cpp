#include <string.h>
#include "cgreen/cgreen.h"

using namespace cgreen;

int sum(int a, int b)
{
	return a + b;
}

Ensure(call_count_is_14)
{
	assert_that(sum(2,2), is_equal_to(4));
	assert_that(sum(5,5), is_not_equal_to(15));
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