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

		git clone https://github.com/kammce/SJSU-Dev.git

**Step 2**
	Change directory into **SJSU-Dev**

	.. code-block:: bash

		cd SJSU-Dev

**Step 3**
	Run :code:`setup` script.

	.. code-block:: bash

		./setup

	.. note::
		This will download and install the gcc-arm binaries, hyperload, telemetry locally to the repo.
		It will also generate the environment variables file and link the makefile and environment file to
		all of the default folders.

Building and Loading Hello World Application
----------------------------------------------

**Step 0**
	From the root of the repository

	.. code-block:: bash

		cd firmware/HelloWorld

**Step 1**
	Source the :code:`env.sh`. You only need to do this once for each terminal session. After sourcing, the necessary environment variables will be added to your shell.

	.. code-block:: bash

		source env.sh

**Step 2**
	Run :code:`make build` within the HelloWorld folder to compile it into a HEX file located in the :code:`bin` folder.

	.. code-block:: bash

		make build

	.. note::
		use the :code:`--help` argument to get additional information on how to use the build script.

**Step 3**
	To load the hex file into your SJOne file you will use the :code:`hyperload.py` file. Run the following:

	.. code-block:: bash

		make flash

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