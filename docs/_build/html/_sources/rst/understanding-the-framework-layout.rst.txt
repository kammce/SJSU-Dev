Understanding The Framework Layout
====================================

File Hierarchy
--------------------------------------

.. code-block:: bash

	firmware
	└── default
	    ├── applications
	    │   ├── FreeRTOS
	    │   │   ├── _can_dbc
	    │   │   └── L5_Application
	    │   │       ├── examples
	    │   │       ├── periodic_scheduler
	    │   │       └── source
	    │   │           └── cmd_handlers
	    │   └── HelloWorld
	    │       ├── _can_dbc
	    │       └── L5_Application
	    │           ├── examples
	    │           ├── periodic_scheduler
	    │           └── source
	    │               └── cmd_handlers
	    ├── bin
	    │   ├── FreeRTOS
	    │   └── HelloWorld
	    ├── lib
	    │   ├── _can_dbc
	    │   ├── L0_LowLevel
	    │   ├── L1_FreeRTOS
	    │   ├── L2_Drivers
	    │   ├── L3_Utils
	    │   ├── L4_IO
	    │   └── newlib
	    └── obj
	        ├── lib
	        ├── FreeRTOS
	        └── HelloWorld

Folder: :code:`firmware`
--------------------------
This folder is meant to hold **projects**. **default** is, understandable, the default project setup.

Folder: :code:`firmware/default`
----------------------------------
.. important::
	This is how you start a new project.

If you want to change, modify, or update files in the :code:lib folder, then it is **RECOMMENDED** for you to make a new project by copying and renaming the default folder to something else. Example: renaming the new folder to :code:`cmpe146` to hold all of your course work that could result in changing the lib files.

Making new projects is helpful, because, the default folder is the one that is modified when there is a new feature added to the repository. To keep your changes, make a new folder.

.. note::
	If you would like to contribute to this project, then editing the lib files in the default folder is permitted.

Folder: :code:`firmware/default/bin`
-------------------------------------
This folder holds the executables that can be loaded into the SJOne board :code:`.hex` as well as a disassembly file :code:`.lst`, linker file :code:`.map` and the  Executable and Linkable Format :code:`.elf` file.

Folder: :code:`firmware/default/application`
---------------------------------------------
This folder holds all of the applications for a given project. Applications use the same base libraries but have different files for using them. Majority of code should be written here.

.. important::
	This is how you start a new application.

To **start** a new application, copy the **FreeRTOS** or **HelloWorld** (depending on what you want to do) folder and rename it to the name of your application.

Folder: :code:`firmware/default/application/<application>/_can_dbc`
--------------------------------------------------------------------
The :code:`_can_dbc` folder holds the CAN message description files and header generator.

Folder: :code:`firmware/default/application/<application>/L5_Application`
--------------------------------------------------------------------------
The :code:`L5_Application` folder holds the :code:`main.cpp` file and other application layer files.

Folder: :code:`firmware/default/lib`
-------------------------------------
This folder holds the core firmware files for the project, such as abstractions for using GPIO, I2C, UART, Interrupts, etc.

Folder: :code:`firmware/default/L%d_%s`
-----------------------------------------
The folders that start with **L<some number>_<some folder name>** are kind of self explanatory as to what they hold. For example, :code:`L1_FreeRTOS` holds files pertaining to FreeRTOS and the FreeRTOS port files. :code:`L2_Drivers` are device drivers and so on and so forth.

Folder: :code:`firmware/default/obj`
-------------------------------------
This folder holds object files created during the compilation stage of building. They are then all linked together to create an :code:`.elf` file afterwards.
