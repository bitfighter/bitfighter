# Bitfighter Makefile
#######################################
#
# Configuration
#
#
# Some installs of Lua call the lua library by different names, and you may need
# to override the default lua library path.  For the ServerHitch CENTOS installs,
# for example, you will need to specify the lua library on the make command line:
#     LUALIB=/usr/lib/liblua.a
#
#
# To compile Bitfighter with debugging enabled, specify
#     DFLAGS=-DTNL_DEBUG
# on the make command line
#
#
# Building with make on Windows is still higly experiemntal. You will probably need
# to add
#     WFLAGS="-DWIN32 -D_STDCALL_SUPPORTED" THREADLIB= GLUT=-lglut32 INPUT=winJoystick.o
# to the make command line to have any hope of getting it to work!  :-)
#
#
########################################
#
# Building under Windows using MinGW:
#
# You can build Bitfighter under Windows using MinGW by following these steps:
# 1. Download and install MinGW
# 2. Follow the steps in the readme in the win_include_do_not_distribute folder
# 3. Build the program using mingw32-make
# 4. Need to figure out linking to GLUT
# DOESN'T FULLY WORK
#
########################################
#
# Here are the steps needed to install on a fresh Ubuntu install:
#
# Download the source code from SVN repository
# svn co https://zap.svn.sourceforge.net/svnroot/zap/trunk bitfighter
#
# Install g++
# Install freeglut-dev and liblua5.1-0
#
# change to root bitfighter folder (where you downloaded the code from SVN)
# build game ==> make
# 
# copy sfx, levels, screenshots, and robots folders from installer folder into exe folder
# 
# run game!!
#
#
# Building for Mac:
#


dedicated:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
	@$(MAKE) -C master
	@$(MAKE) -C zap	dedicated

default:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
	@$(MAKE) -C master
	@$(MAKE) -C zap	

zap:
	@$(MAKE) -C zap

.PHONY: clean 

clean:
	@$(MAKE) -C tnl clean
	@$(MAKE) -C libtomcrypt clean
	@$(MAKE) -C master clean
	@$(MAKE) -C zap	clean

cleano:
	@$(MAKE) -C tnl cleano
	@$(MAKE) -C libtomcrypt cleano
	@$(MAKE) -C master cleano
	@$(MAKE) -C zap	cleano

docs:
#	@$(MAKE) -C docs

