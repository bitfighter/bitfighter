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
#
# Here are the steps needed to install on a fresh Ubuntu install:
#
# Download the source code from SVN repository
# svn co https://zap.svn.sourceforge.net/svnroot/zap bitfighter
#
# Install g++
# sudo apt-get install g++
#
# Install freeglut
# sudo apt-get install freeglut3            (maybe not needed)
# sudo apt-get install freeglut3-dev
#
# Install OpenGL (maybe not needed)
# sudo apt-get install libgl1-mesa-dev
#
# Install libreadline lib, present on most Linux installs, absent on Ubuntu
# sudo apt-get install libreadline5-dev
#
# build lua.a:
#       download source ==> wget http://www.lua.org/ftp/lua-5.1.4.tar.gz
#       uncompress ==> tar zxf lua-5.1.4.tar.gz
#       compile ==> make linux
#       move liblua.a to Bitfighter's lua folder ==> mv src/liblua.a ../lua
#       You can now remove all the other lua stuff if you want
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

default:
	@$(MAKE) -C tnl
	@$(MAKE) -C libtomcrypt
	@$(MAKE) -C master
#	@$(MAKE) -C md5
	@$(MAKE) -C zap	

zap:
	@$(MAKE) -C zap

.PHONY: clean 

clean:
	@$(MAKE) -C tnl clean
	@$(MAKE) -C libtomcrypt clean
	@$(MAKE) -C master clean
#	@$(MAKE) -C md5	
	@$(MAKE) -C zap	clean

docs:
#	@$(MAKE) -C docs

