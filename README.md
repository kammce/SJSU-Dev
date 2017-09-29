# SJSU-DEV-Linux

[![SJSU-Dev-Linux Version](https://img.shields.io/badge/version-1.0.0-blue.svg)](https://github.com/kammce/SJSU-DEV-Linux)
[![GNU General Public License v2.0 Badge](https://img.shields.io/badge/license-GNUv2.0-blue.svg)](https://github.com/kammce/SJSU-DEV-Linux)
[![Documentation Status](https://readthedocs.org/projects/sjsu-dev-linux/badge/?version=latest)](http://sjsu-dev-linux.readthedocs.io/en/latest/?badge=latest)
[![Build Status](https://travis-ci.org/kammce/SJSU-DEV-Linux.svg?branch=master)](https://travis-ci.org/kammce/SJSU-DEV-Linux)

## Operating System Supported

<p align="center">
<img src="http://design.ubuntu.com/wp-content/uploads/ubuntu-logo32.png" height="100px"/>
&nbsp;&nbsp;
<img src="http://cdn.osxdaily.com/wp-content/uploads/2010/10/giant-apple-logo-bw.png" height="100px" />
&nbsp;&nbsp;
<img src="https://cdn.worldvectorlogo.com/logos/microsoft-windows-22.svg" height="100px" />
</p>

Built for **Debian** based systems, **Mac OSX** (beta), and **Windows 10** using the latest Windows Linux Subsystem (WLS) Insider Builds. The instructions to install on Windows BASH are the same as in Linux.

## Tutorial

See **[documentation](http://sjsu-dev-linux.readthedocs.io/en/latest/?badge=latest)** for a full tutorial.

## Contrib
* [Preet Kang](http://www.socialledge.com/sjsu/index.php?title=Main_Page): Original creator of the SJDev development framework.
* [Khalil Estell](http://kammce.io): Creator of the Linux port of SJDev and maintainer of this repo.
* [Kai Wetlesen](https://github.com/kaiwetlesen): Contributed Mac OS X port and development environment improvements.

### Special Credits
* **Mikko Bayabo**: Windows surface destructive testing
* **WSL testing**: Sameer Azer, Aaron Moffit, Ryan Lucus

## Change Log

### Update 09.28.2017

* Added support for Windows 10 WLS and Mac OS X (beta)
* Setup script no longer pollutes global space (except for gdbgui)
* Setup script now generates an environment script to set environment variables
* Massive change to file hierarchy
	* Firmware folders can be moved to any other location as long as it has a link to the development
	* Default firmware folders come with a symbolic link to `env.sh` and `makefile`.
	* There is no longer a defaults folder, application folders were moved to firmware folder.

### Update 09.11.2017

* Added **SJOne_lpc1758_rev4.pdf**


<!--

apt-get install python-sphinx
pip install sphinx

sudo pip install breathe
sudo apt-get install doxygen

https://github.com/Velron/doxygen-bootstrapped

-->
