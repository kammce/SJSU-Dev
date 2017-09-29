Getting Started
=================

Prerequisites
---------------
Need a running version of Ubuntu 14.04 LTS or above. Ubuntu in a virtual machine such as VirtualBox or VMPlayer will work as well.

.. note::

	The it is possible to get SJSU-Dev-Linux to work completely on Windows and Mac OSX if you have all of the necessary PATH dependencies installed, but that is not covered here. You will need to manually install all of the necessary components in the installer script.

Installation
-------------

**Step 1**
	Clone the repository

	.. code-block:: bash

		git clone --recursive https://github.com/kammce/SJSU-DEV-Linux.git

**Step 2**
	Change directory into **SJSU-Dev-Linux**

	.. code-block:: bash

		cd SJSU-DEV-Linux

**Step 3**
	Run :code:`setup` script.

	.. code-block:: bash

		./setup

	.. warning::

		Do not run this script using **SUDO**. The script will ask you for **sudo** permissions once it runs.

	.. note::
		This will install gtkterm, mono-complete, and gcc-arm-embedded packages

Building and Loading Hello World Application
----------------------------------------------

**Step 1**
	From the root of the repository

	.. code-block:: bash

		cd firmware/default

**Step 2**
	Run :code:`build` script. A HEX file :code:`bin/HelloWorld/HelloWorld.hex` and subsequent folders should have been created after this script finishes.

	.. code-block:: bash

		./build HelloWorld

	.. note::
		use the :code:`--help` argument to get additional information on how to use the build script.

**Step 3**
	To load the hex file into your SJOne file you will use the :code:`hyperload.py` file. Run the following:

	.. code-block:: bash

		./hyperload.py /dev/ttyUSB0 bin/HelloWorld/HelloWorld.hex

	The first argument is the path to the serial device. The second argument is the hexfile to load into the SJOne board.

**Step 4**
	To view serial output, run GTKTerm by using the following command:

	.. code-block:: bash

		gtkterm -p /dev/ttyUSB0 -s 38400

	*How to use GTKTerm*
		1. Set *CR LF Auto* to true by going to the :code:`Main Menu > Configuration > CR LF Auto` and click on it.
		2. Press :code:`F8` (Clears RTS signal), then press :code:`F7` (Clears DTR signal) to start SJOne.
		3. You should see the board counting up on the 7-Segment display and in binary on the LEDs.

**Step 5**
	Done!!

Building and Loading FreeRTOS Project
---------------------------------------
Instructions are the same as HelloWorld, but you need to change the run the build script's last argument to *FreeRTOS* rather than HelloWorld.