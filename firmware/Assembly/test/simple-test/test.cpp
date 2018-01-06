#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "fff.h"

DEFINE_FFF_GLOBALS;

int sum(int a, int b)
{
	return a + b;
}

TEST_CASE( "Testing Unittest", "[test]" )
{
	SECTION( "Summation test 0" )
	{
		  CHECK( sum(2,2) == 4 );
		REQUIRE( sum(5,5) != 15 );
	}
}