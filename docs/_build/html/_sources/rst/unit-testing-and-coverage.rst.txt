Unit Testing and Code Coverage
=================================

Unit Testing Tools
-------------------
RoverCore-S uses the following the libraries:

#. `Mocha <https://mochajs.org/>`_ as a unit testing framework
#. `Chai <http://chaijs.com/>`_ as an assertion library
#. `Sinon <http://sinonjs.org/>`_ as a stubs library.
#. `JSHint <http://jshint.com/>`_ as a JS linter
#. `Istanbul <https://istanbul.js.org/>`_ as the test coverage library.
#. `Grunt <https://gruntjs.com/>`_ to run the tests, linter, and coverage

These libraries are installed when you run the ``./install.sh`` file.

Creating a Unit Test
----------------------

To create a new unit test, copy the ``Example.js`` file in ``/test/modules/`` into another file and name it the same name as your lobe. The following is the contents of the example unit test.

.. code-block:: JavaScript

	"use strict";

	var INSERT_LOBE_NAME_HERE = require("Protolobe/Protolobe");

	describe("Testing INSERT_LOBE_NAME_HERE Class", function()
	{
		var util = require("test_lobe_utilities");

		var test_unit = new INSERT_LOBE_NAME_HERE(util);

		describe("Testing Lobe Methods", function()
		{
			it("#react() test here", function()
			{
				test_unit.react();
				expect(true).to.be.true;
			});
			it("#halt() test here", function()
			{
				test_unit.halt();
				expect(true).to.be.true;
			});
			it("#resume() test here", function()
			{
				test_unit.resume();
				expect(true).to.be.true;
			});
			it("#idle() test here", function()
			{
				test_unit.idle();
				expect(true).to.be.true;
			});
			it("#additionalMethod() test here", function()
			{
				expect(true).to.be.true;
			});
		});
	});

Sinon, Chai, and Mocha are all global when you test your program. The folders ``core``, ``modules``, ``utilities``, and ``test`` have all been added to the require path. So requiring them does not require the whole relative path to the files.

.. important::
	Make sure to always include tests for the ``react``, ``halt``, ``resume``, and ``idle`` methods of your lobe.

For more information for on unit testing using mocha, chai and sinon see the following:

* https://mochajs.org/
* http://sinonjs.org/
* http://chaijs.com/guide/styles/#expect

Testing Commands
------------------

To run a single and specific unit test is the following command

.. code-block:: bash

	mocha test/modules/<unit test file>.js

To run all unit tests (but not the linter)

.. code-block:: bash

	grunt unittest

To run just the linter

.. code-block:: bash

	grunt lint

Run everything except for istanbul code coverage

.. code-block:: bash

	grunt --force

To run everything, just use

.. code-block:: bash

	npm test

To generate a code coverage report, run this in the root of the project

.. code-block:: bash

	bash <(curl -s https://codecov.io/bash)

