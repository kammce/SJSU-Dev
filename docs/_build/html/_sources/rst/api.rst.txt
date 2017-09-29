Lobe API and System Features
=============================

Utility Classes
----------------

Log
::::
Abstraction library for printing out debug and log information to STDOUT and to file. Log will format the messages in the following way to make seeing the output of a particular lobe easier.

    ``[ < timestamp > ][ < Lobe Name > ] :: < output message >``

Usage:

.. code-block:: JavaScript

    this.log.output(msg_to_output, ...);
    this.log.output("HELLO WORLD", { foo: "bar" });

feedback()
:::::::::::
The feedback function will send information back to mission control from the lobe. It works similar to console.log, in that it allows a variable number of parameters.

Usage:

.. code-block:: JavaScript

    this.feedback(msg_to_output, ...);
    this.feedback("HELLO WORLD", { foo: "bar" });

Model
::::::
Collection of all information stored on the rover by the lobes. Lobes can use the information stored in this structure to get information that other lobes have stored. For example, if the drive system lobe needed compass heading information and a compass lobe has already stored information there, then the following can happen:

.. code-block:: JavaScript

	//// Compass.js lobe
	this.model.registerMemory("Compass");
	this.model.set("Compass",
	{
	    heading: 45 // in degrees
	});

	//// DriveSystem.js lobe
	var compass = this.model.get("Compass");
	if(compass["heading"] < SOME_VALUE)
	{
	    DoAThing();
	}

If a lobe would like to send such information to mission control, rather than using *feedback* it can be done through the model. Every time ``this.model.set()`` is used the information is automatically sent to mission control.

If you need to return the whole database of information one could use ``var memories = getMemory(0);``

**var memories** will contain the whole structure of the model:

.. code-block:: JavaScript

	{
	    "Compass": { heading: 45 },
	    "DriveSystem":
	    {
	        speed: 20,
	        angle: 45
	    },
	    ...
	}

upcall()
:::::::::
Upcalls can be used to make calls to Cortex to do specific things.

* Calling another module's REACT function

.. code-block:: JavaScript

	this.upcall("CALL", "<target>", { /* command structure */ })

* HALTALL, RESUMEALL or IDLEALL Modules

.. code-block:: JavaScript

	this.upcall("HALTALL");
	this.upcall("IDLEALL");
	this.upcall("RESUMEALL");

* Shutdown System (Computer)

.. code-block:: JavaScript

	this.upcall("SYSTEM-SHUTDOWN");

* Reboot System (Computer)

.. code-block:: JavaScript

	this.upcall("SYSTEM-RESTART");

* Restart-Cortex

.. code-block:: JavaScript

	this.upcall("RESTART-CORTEX");