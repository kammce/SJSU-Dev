#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"
#include "fff.h"
#include "MaxElements.hpp"

DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC(vChipSelect, bool);
FAKE_VALUE_FUNC(uint8_t, vSPITransfer, uint8_t);

MAX7456 Max(vChipSelect, vSPITransfer);

TEST_CASE( "Testing MAX Initialization", "[max]" )
{
	SECTION( "init 0" )
	{
		vSPITransfer_fake.custom_fake = [](uint8_t Data)
		{
		    printf("[STUB] Transfer = %c\n", Data);
		    return Data;
		};

		Max.Initialize();

		printf("ChipSelect_fake.call_count = %d\n", vChipSelect_fake.call_count);
		printf("SPITransfer_fake.call_count = %d\n", vSPITransfer_fake.call_count);

		// CHECK( vChipSelect_fake.call_count > 14 );
		CHECK( vChipSelect_fake.call_count <= 14 );
	}
	SECTION( "init 1" )
	{
		vSPITransfer_fake.custom_fake = [](uint8_t Data)
		{
		    printf("[NEW STUB] Transfer = %c\n", Data);
		    return Data;
		};

		Max.Initialize();

		printf("ChipSelect_fake.call_count = %d\n", vChipSelect_fake.call_count);
		printf("SPITransfer_fake.call_count = %d\n", vSPITransfer_fake.call_count);

		REQUIRE( vChipSelect_fake.call_count <= 28 );
	}
}