## Drivers that are initialized

* Minimal, Polling UART0 initialized @ **38400** BPS
* RTC
* ADC
* SPI0, SPI1
* I2C2

## Other items that are setup

* PLL Setup according to desired CPU speed set @ sys_config.h
* Flash Acceleration setup according to CPU frequency
* FreeRTOS fully setup, just need to create tasks and start FreeRTOS scheduler
* Attempt is made to mount SD Card & Flash Memory, so File IO is ready

## The following resources are setup before main() is called

* System Tick Interrupt is setup for FreeRTOS
* Timer1 is enabled to to provide system timer services that also helps capture IR signal. Timer1 also services the mesh networking task if FreeRTOS is not running. *

## Layer 0 sets up the following items

* Setup the System Memory and CPU Clock
* Initialize minimal UART0 (polling version) for standard I/O
* Make a call to high_level_init() followed by call to main()
* This layer also contains the processor memory map file

## Layer 1 provides the FreeRTOS API

* Some source code is optimized and improved to provide better information.

## Layer 2 provides the System Drivers

* All of your drivers should be placed in this layer.
* Base classes for UART and I2C are present to help write drivers for additional UARTs

## Layer 3 provides Utility functions and classes

* Utility functions contain embedded tweaks for String and Vector class
* Other classes include LOGGER, Command Handler, and FreeRTOS wrapper around Timers

## Layer 4 provides gateway to the SJ One Board IO that includes

* FAT File System to access SD Card and on-board Flash Memory
* Nordic wireless API also exists at this layer
* Storage class that allows to format and get information about SD and Flash Memory
* Acceleration, IR, Temperature, and Light Sensors
* LED Display, LEDs, and Switches class

## Layer 5 is where your application code should go including

* FreeRTOS Tasks
* High level code that interacts with lower layers