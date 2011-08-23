# Bitfighter Makefile
#######################################
#
# Configuration
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
# Here are the steps needed to install on a fresh Ubuntu install:
#
# Download the source code from SVN repository
# svn co https://zap.svn.sourceforge.net/svnroot/zap/trunk bitfighter
#
# Install g++
# Install freeglut-dev, libopenal-dev, libalut-dev
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

default: release

release:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
	#@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src
	@$(MAKE) -C alure
	@$(MAKE) -C zap	

debug:
	@$(MAKE) -C tnl debug
	@$(MAKE) -C libtomcrypt
	#@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src
	@$(MAKE) -C alure
	@$(MAKE) -C zap	debug

dedicated:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
	#@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src 
	@$(MAKE) -C zap	dedicated

dedicated_debug:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
	#@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src 
	@$(MAKE) -C zap	dedicated_debug

bitfighter:
	@$(MAKE) -C zap

master:
	@$(MAKE) -C master

.PHONY: clean 

clean:
	@$(MAKE) -C tnl clean
	@$(MAKE) -C libtomcrypt clean
	@$(MAKE) -C master clean
	@$(MAKE) -C lua/lua-vec/src clean
	@$(MAKE) -C alure clean
	@$(MAKE) -C zap	clean

cleano:
	@$(MAKE) -C tnl cleano
	@$(MAKE) -C libtomcrypt cleano
	@$(MAKE) -C master cleano
	@$(MAKE) -C lua/lua-vec/src cleano
	@$(MAKE) -C alure cleano
	@$(MAKE) -C zap	cleano

