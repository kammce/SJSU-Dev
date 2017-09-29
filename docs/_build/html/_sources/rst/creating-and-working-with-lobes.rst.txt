Creating and Working with Lobes
=================================

Lobes in RoverCore-S and What They Do
--------------------------------------

The Lobes of RoverCore-S are modules that do work on the system. Lobes are structured classes that give mission control a means of controlling a specific system of the robot. Lobes can be used to retrieve and store sensor data, stream cameras, send an email, or literally do anything else that a Linux system running node.js can do.

How Lobes Works
----------------
Lobe States
::::::::::::
Lobes have three states: **HALTED**, **RUNNING**, and **IDLING**. All of these states have a method associated with it. Each must be defined but does not necessary need to do anything. They can be empty methods and just return true.

#react() method
::::::::::::::::
When Cortex receives a command from mission control *targets* a specific Lobe, Cortex will call that lobe's *react()* method with the first parameter being the command sent from mission control. Thus the *react()* method is a means of handling commands sent from mission control to your lobe. Only one parameter is given to the react() method, but the command can be a mixed type (string, integer, structure, etc.). It is up to the lobe and mission control interface designer to decide how the commands will be represented.

#halt() method & HALTED state
::::::::::::::::::::::::::::::
In the HALTED state, the lobe is stopped from doing any work and kept from reacting from mission control signals until RUNNING. Cortex will attempt to halt the a lobe in the following situations:
1. If the Mission Control controller of a lobe disconnects from the rover server or server proxy.
2. If the Mission Control controller sends a manual halt signal to Cortex to halt the lobe.
3. If another lobe uses an UPCALL to trigger the halt of a specific lobe or all lobes.

The *halt()* method within the lobe is the procedure that is run when Cortex attempts to halt the lobe. Return true if the halt was successful. Return false if the lobe did not halt successfully. Take care to use this area wisely.

.. warning::

	For critical systems, it may be very important to stop the actions of the arm or wheels when the module halts, so be sure to do so in these procedures. TRY NOT TO FAIL AT THIS!

#resume() method & RUNNING state
::::::::::::::::::::::::::::::::::
In the RUNNING state, the lobe is active. The only way to exit a HALTED state is to run the *resume()* method. The resume method should do what ever is needed to bring the lobe out of the halted state.
1. If the Mission Control controller sends a manual resume signal to Cortex to resume a halted lobe.
2. If another lobe uses an UPCALL to trigger resume of a specific lobe or all lobes.

#idle() method & IDLING state
::::::::::::::::::::::::::::::
Lobes are put into an IDLING state if they have not been sent a command from mission control in the specified amount defined in the lobe constructor. The **default** value for this is ``2000ms`` by the base class Neuron.js.

.. note::

	This is useful when all connections to mission control are set, but something that is used for sending messages fails to send. This acts as a backup to idle or stop the system when this occures. This could also be used to sleep the system.

Creating a new Lobe
---------------------

**Step 1**
	Decide on a name for your lobe. Lets call this name **NewLobe** for the following steps. Naming convention for lobes is **CamelCase**


**Step 1**
	Go into the modules folder and make a copy of the template folder **Protolobe**. Rename that folder to **NewLobe**.

**Step 2**
	Within your lobe folder, rename the **Protolobe.js** file to **NewLobe.js**.

**Step 3**
	Change the name of the **Class** and the last line ``module.export = ClassName`` from **Protolobe** to **NewLobe**. Should look like the following:

.. code-block:: JavaScript

	"use strict";

	var Neuron = require('../Neuron');

	class NewLobe extends Neuron // changed Protolobe to name of folder
	{
	    constructor(util)   { ... }
	    react(input)        { ... }
	    halt()              { ... }
	    resume()            { ... }
	    idle()              { ... }
	}

	module.exports = NewLobe; // changed Protolobe to name of folder

**Step 4**
	Update the following two lines in the Constructor

.. code-block:: JavaScript

	//// Set lobe color by changing this line
	this.log.setColor("red");
	//// Time in milliseconds before idle timeout
	this.idle_timeout = 2000;

The list of text colors can be found here https://www.npmjs.com/package/colors.

*util* constructor parameter properties
-------------------------------------------
Each lobe is given a utilities structure by Cortex through their constructor. The structure has the following properties:

* **name**: The name of the module using the module's folder name
* **log**: holds the log object reference
* **model**: holds the model object reference
* **upcall**: holds the Cortex upcall function reference
* **extended**: holds the structure of the extended utilities
* **feedback**: holds the feedback function reference