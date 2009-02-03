#
# Building under Windows using MinGW:
#
# You can build bitFighter under Windows using MinGW by following these steps:
# 1. Download and install MinGW
# 2. Follow the steps in the readme in the win_include_do_not_distribute folder
# 3. Build the program using mingw32-make
# 4. Need to figure out linking to GLUT
#
#
# Building under Linux:
#
# Do some stuff
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

