Unit Testing
=============

Unit Testing Tools
-------------------
To unit test we support CGreens and Catch along with the FFF.h (fake function framework). CGreen must be installed globally your system for the makefile to work.

Files in test folder
---------------------
Within the test folder, there will be a folder named **simple-test** which contains the following:

	* **makefile**: for building the tests
	* **test.cpp**: a Catch unit test example.
	* **cgreen-test.cpp**: is a CGreen test example.
	* **test-files.list**: a new line delimited list of files to be included in your test.

Compiling and Running Tests
----------------------------

Enter the **simple-test** folder and run :code:`make` to compile and run the Catch unit test and :code:`make cgreen` to compile and run the CGreen unit test file.

Created additional unit tests
------------------------------
Copy the **simple-test** folder and edit the files within to make a new test. Do not move the folder out of the test directory of the application.

Stubbing out and Spying on Function calls using FFF.h framework
-----------------------------------------------------------------
In your source, prefix the function you want to test with the following :code:`__attribute__((weak))`. This will make it a weak function that can be overridden with another definition of the same function. Within your test files you can include the following to create a function stub/spy:

	.. code-block:: C++

		FAKE_VOID_FUNC(functionToStubAndSpyOn, bool);

		/* ... */

		functionToStubAndSpyOn.custom_fake = []()
		{
		    printf("Custom function");
		};

See more here https://github.com/meekrosoft/fff.