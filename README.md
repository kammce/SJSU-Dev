# SJSU-Dev

[![SJSU-Dev-Linux Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/kammce/SJSU-DEV-Linux)
[![GNU General Public License v2.0 Badge](https://img.shields.io/badge/license-GNUv2.0-blue.svg)](https://github.com/kammce/SJSU-Dev)
[![Documentation Status](https://readthedocs.org/projects/sjsu-dev/badge/?version=latest)](http://sjsu-dev.readthedocs.io/en/latest)
[![Build Status](https://travis-ci.org/kammce/SJSU-Dev.svg?branch=master)](https://travis-ci.org/kammce/SJSU-Dev)

## Operating System Supported

<p align="center">
<img src="https://assets.ubuntu.com/v1/29985a98-ubuntu-logo32.png" height="100px"/>
&nbsp;&nbsp;&nbsp;&nbsp;
<img src="http://cdn.osxdaily.com/wp-content/uploads/2010/10/giant-apple-logo-bw.png" height="100px" />
&nbsp;&nbsp;&nbsp;&nbsp;
<img src="https://cdn.worldvectorlogo.com/logos/microsoft-windows-22.svg" height="100px" />
</p>

Built for **Debian** based systems, **Mac OSX**, and **Windows 10** using the latest Windows Linux Subsystem (WLS) Insider Builds. The instructions to install on Windows BASH are the same as in Linux.

## Tutorial

See **[documentation](http://sjsu-dev.readthedocs.io/en/latest/?badge=latest)** for a full tutorial.

## Contrib
* [Preet Kang](http://www.socialledge.com/sjsu/index.php?title=Main_Page): Original creator of the SJDev development framework.
* [Khalil Estell](http://kammce.io): Creator of the Linux port of SJDev and maintainer of this repo.
* [Kai Wetlesen](https://github.com/kaiwetlesen): Contributed Mac OS X port and development environment improvements.
* [Anahit Sarao](https://github.com/s3nu): Major updates to initial Mac and Linux ports.

### Special Credits
* **Mikko Bayabo**: Windows surface destructive testing
* **WSL testing**: Sameer Azer, Aaron Moffit, Ryan Lucus

## Change Log

### Update 01.05.2018

* I2C library extended to allow for single byte send and receive.
* Added Assembly project template to repo
* Fixed issue with HelloWorld project template

### Update 10.23.2017

* Closes #10 : Makefile threading causing problems bug
* Closes #9 : Output warning if you user does not source env.sh file problem
* Closes #1 : Docs do not fit current repo problem

### Update 09.30.2017
* Added rules into make file to allow for assembly compilation.
* Added in sample Assembly folder to demonstrate making a project in assembly in SJSU-Dev.
* Created a declassify c++ file as a space to convert C++ method calls to global functions.

### Update 09.28.2017

* Added support for Windows 10 WLS and Mac OS X (beta)
* Setup script no longer pollutes global space (except for gdbgui)
* Setup script now generates an environment script to set environment variables
* Massive change to file hierarchy
	* Firmware folders can be moved to any other location as long as it has a link to the development
	* Default firmware folders come with a symbolic link to `env.sh` and `makefile`.
	* There is no longer a defaults folder, application folders were moved to firmware folder.
* Added unit testing frameworks cgreen and catch along with fff.h (fake function framework)

### Update 09.11.2017

* Added **SJOne_lpc1758_rev4.pdf**

### Update 09.30.2017

* Operating system detection.
* Setup checks for dependencies and installs needed packages.


<!--

apt-get install python-sphinx
pip install sphinx

sudo pip install breathe
sudo apt-get install doxygen

https://github.com/Velron/doxygen-bootstrapped

-->
