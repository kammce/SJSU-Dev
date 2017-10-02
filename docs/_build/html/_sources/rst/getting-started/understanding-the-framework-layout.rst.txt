Understanding The Framework Layout
====================================

File Hierarchy
--------------------------------------

.. code-block:: bash

	firmware
	├── HelloWorld
	│   ├── _can_dbc
	│   ├── L5_Application
	│   └── test
	│       └── simple-test
	├── FreeRTOS
	│   ├── bin
	│   ├── _can_dbc
	│   ├── L5_Application
	│   │   ├── examples
	│   │   ├── periodic_scheduler
	│   │   └── source
	│   ├── obj
	│   └── test
	│       └── simple-test
	├── Telemetry
	│   ├── _can_dbc
	│   ├── L5_Application
	│   │   ├── examples
	│   │   ├── periodic_scheduler
	│   │   └── source
	│   └── test
	│       └── simple-test
	├── Assembly
	│   ├── bin
	│   ├── obj
	│   └── test
	│       └── simple-test
	├── Unittest
	│   ├── _can_dbc
	│   ├── L5_Application
	│   │   ├── examples
	│   │   ├── periodic_scheduler
	│   │   └── source
	│   └── test
	│        └── Function-Stubbing
	└── lib
	    ├── _can_dbc
	    ├── L0_LowLevel
	    │   └── source
	    ├── L1_FreeRTOS
	    │   ├── hooks
	    │   ├── include
	    │   ├── MemMang
	    │   ├── portable
	    │   ├── src
	    │   └── trace
	    ├── L2_Drivers
	    │   ├── base
	    │   └── src
	    ├── L3_Utils
	    │   ├── src
	    │   └── tlm
	    ├── L4_IO
	    │   ├── fat
	    │   ├── src
	    │   └── wireless
	    ├── L6_Testing
	    └── newlib

Folder: :code:`firmware`
--------------------------
This folder is meant to hold the firmware **applications** you make.
The default applications are:

	* **HelloWorld**: Template for simple single-threaded applications. (CMPE 30)
	* **FreeRTOS**: templatT FreeRTOS application. (CMPE 127/146/243/244)
	* **Telemetry**: Template program to demonstrate usage of Telemetry and FreeRTOS. (CMPE 127/146/243/244)
	* **Assembly**: Template application for building simple ARM assembly programs (CMPE 102)
	* **Unittest**: Template unit testing application. (CMPE 146/146/243/244)

Folder: :code:`firmware/<application>/L5_Application`
--------------------------------------------------------------------------
The :code:`L5_Application` folder holds the :code:`main.cpp` file and other application layer files.

Folder: :code:`firmware/<application>/L5_Assembly`
--------------------------------------------------------------------------
The :code:`L5_Assembly` folder holds the :code:`main.S` ARM assembly template program.

Folder: :code:`<applications>/bin`
-------------------------------------
This folder holds the executables that can be loaded into the SJOne board :code:`.hex`. It also holds the disassembly file :code:`.lst`, linker file :code:`.map` and the Executable and Linkable Format :code:`.elf` file.

Folder: :code:`<application>/_can_dbc`
--------------------------------------------------------------------
The :code:`_can_dbc` folder holds the CAN message description files and header generator.

Folder: :code:`<application>/obj`
-------------------------------------
This folder holds object files created during the compilation stage of building. They are then all linked together to create an :code:`.elf` file afterwards. The last phase converts the :code:`.elf` to a :code:`.hex` file to be loaded into SJ-One's flash memory.

Folder: :code:`<application>/test`
-------------------------------------
This folder holds object files created during the compilation stage of building. They are then all linked together to create an :code:`.elf` file afterwards. The last phase converts the :code:`.elf` to a :code:`.hex` file to be loaded into SJ-One's flash memory.

Folder: :code:`firmware/lib`
-------------------------------------
This folder holds the core firmware files for the SJ-One file, such as abstractions for using GPIO, I2C, UART, Interrupts, etc.

Folder: :code:`firmware/lib/L%d_%s`
-----------------------------------------
The folders that start with **L<some number>_<some folder name>** are kind of self explanatory as to what they hold. For example, :code:`L1_FreeRTOS` holds files pertaining to FreeRTOS and the FreeRTOS port files. :code:`L2_Drivers` are device drivers and so on and so forth.
