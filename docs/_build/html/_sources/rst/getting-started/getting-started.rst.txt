Getting Started
=================

Prerequisites
---------------
Need a running version of Ubuntu 16.04 LTS or above, or OS X, or Windows 10 with WSL with insider builds installed.

You must also have the following pre-installed:

	* Python 2.7+
	* Pip
	* Git

Installation
-------------

**Step 0**
	Clone the repository

	.. code-block:: bash

		git clone https://github.com/kammce/SJSU-Dev.git

**Step 1**
	Change directory into **SJSU-Dev**

	.. code-block:: bash

		cd SJSU-Dev

**Step 2**
	Run :code:`setup` script.

	.. code-block:: bash

		./setup

	.. note::
		This will download and install the gcc-arm binaries, hyperload, telemetry locally to the repo.
		It will also generate the environment variables file and link the makefile and environment file to
		all of the default folders.

**Step 3**
	Edit the :code:`env.sh` script. Change the line :code:`SJSUONEDEV=/dev/ttyUSB0` to equal what you have on your system.

	**How to find your serial device on Ubuntu**

	You probably do not have to change anything.

	If no other devices are connected to your machine, then it will be :code:`/dev/ttyUSB0`. It is recommended to keep it at this value, because when you add more devices, it will increment to :code:`/dev/ttyUSB1`. Once you remove your devices and replace them, the value will reset.

	**How to find your serial device on Mac OS X**

		1. Remove the SJ-One from your computer if it is connected.
		2. List the files in the :code:`/dev` folder by running the following :code:`ls /dev/`.
		3. Plug it into your computer and run :code:`ls /dev/`.
		4. Observe the new file that was created.
		5. On mac, the path should look something like the following :code:`/dev/tty.cumodemfd1337`.
		6. If so, change the line in env.sh to that file path from :code:`SJSUONEDEV=/dev/ttyUSB0` -> :code:`SJSUONEDEV=/dev/tty.`

	**How to find your serial device on Windows Linux Subsystem**

	On Windows it should be :code:`/dev/ttyS3`. Check your device manager to see what number COM device your device. The number after COM is the number after the **S** in the :code:`/dev/ttyS` string. That is your device. Replace the line :code:`SJSUONEDEV=/dev/ttyUSB0` -> :code:`SJSUONEDEV=/dev/ttyS`

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
	To load the hex file into your SJ-One, run the following make command:

	.. code-block:: bash

		make flash

	.. note::
		If you run this command without first building, this command will build your project and then flash it. So you can skip the step above if you like.

**Step 4**
	To view serial output, and interact with the board, run the following make command:

	.. code-block:: bash

		make telemetry

	.. note::
		The interface will pop up in your default browser on launch, except on Windows. You will need to enter the IP address and port manually.

**Step 5**
	Done!!

Building and Loading FreeRTOS Project
---------------------------------------
Instructions are the same as HelloWorld, but you need to enter the firmware/FreeRTOS folder and run make from there.

Creating your own Project
---------------------------------------
Copy and rename the FreeRTOS or HelloWorld template folders to any place in your computer to make a new project.