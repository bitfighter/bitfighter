# Bitfighter Makefile
#######################################
#
# Your options are:
#  - make [release] 
#  - make debug 
#  - make dedicated
#  - make dedicated_debug 
#
# In addition the master server can be compiled with
#  - make master
# 

default: release

release:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
#	@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src
	@$(MAKE) -C alure
	@$(MAKE) -C zap	

debug:
	@$(MAKE) -C tnl debug
	@$(MAKE) -C libtomcrypt
#	@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src
	@$(MAKE) -C alure debug
	@$(MAKE) -C zap	debug

dedicated:
	@$(MAKE) -C tnl 
	@$(MAKE) -C libtomcrypt
#	@$(MAKE) -C master
	@$(MAKE) -C lua/lua-vec/src 
	@$(MAKE) -C zap	dedicated

dedicated_debug:
	@$(MAKE) -C tnl debug 
	@$(MAKE) -C libtomcrypt
#	@$(MAKE) -C master
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

