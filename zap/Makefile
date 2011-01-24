# Bitfighter Makefile
#######################################
# 
# Configuration
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
#######################################

CC=gcc -g  -I../tnl -I../glut -I../openal -DTNL_ENABLE_LOGGING -W #-O2
THREADLIB= -lpthread
GLUT=-lGL -lGLU -lglut
INPUT=linuxInput.o 

OBJECTS_ZAP=\
   CTFGame.o\
   HTFGame.o\
   IniFile.o\
   loadoutZone.o\
   SweptEllipsoid.o\
   UI.o\
   UIAbstractInstructions.o\
   UIChat.o\
   UICredits.o\
   UIDiagnostics.o\
   UIEditor.o\
   UIEditorInstructions.o\
   UIErrorMessage.o\
   UIYesNo.o\
   UIGame.o\
   UIInstructions.o\
   UIKeyDefMenu.o\
   UITeamDefMenu.o\
   UIMenuItems.o\
   UIMenus.o\
   UINameEntry.o\
   UIQueryServers.o\
   UIGameParameters.o\
   barrier.o\
   config.o\
   controlObjectConnection.o\
   dataConnection.o\
   engineeredObjects.o\
   engineerHelper.o\
   flagItem.o\
   keyCode.o\
   game.o\
   gameConnection.o\
   gameItems.o\
   gameLoader.o\
   gameNetInterface.o\
   gameObject.o\
   gameObjectRender.o\
   gameType.o\
   gameWeapons.o\
   goalZone.o\
   gridDB.o\
   helperMenu.o\
   huntersGame.o\
   input.o\
   item.o\
   lineEditor.o\
   $(INPUT)\
   loadoutHelper.o\
   luaGameInfo.o\
   luaLevelGenerator.o\
   luaObject.o\
   luaUtil.o\
   main.o\
   masterConnection.o\
   move.o\
   moveObject.o\
   oglconsole.o\
   playerInfo.o\
   projectile.o\
   quickChatHelper.o\
   rabbitGame.o\
   retrieveGame.o\
   robot.o\
   screenShooter.o \
   sfx.o\
   ship.o\
   SlipZone.o\
   soccerGame.o\
   sparkManager.o\
   speedZone.o\
   statistics.o\
   stringUtils.o\
   teamInfo.o\
   teleporter.o\
   textItem.o\
   voiceCodec.o\
   zoneControlGame.o\
   gsm_encode.o\
   gsm_decode.o\
   gsm_state.o\
   lpc10enc.o\
   lpc10dec.o\
   md5wrapper.o\
   md5.o\
   BotNavMeshZone.o\
   ../master/masterInterface.o


CFLAGS=
DFLAGS=
EXEFILE=bitfighter
OPENAL=-lopenal -lalut
WFLAGS=
LFLAGS=-lstdc++ -lm -ldl -lX11

.c.o:
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) $<

.cpp.o : 
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) $<

default: bitfighter


dedicated: CFLAGS=-DZAP_DEDICATED 
dedicated: LFLAGS=-lstdc++ -lm -ldl
dedicated: GLUT=
dedicated: OPENAL=
dedicated: EXEFILE=bitfighterd
dedicated: bitfighter


bitfighter: $(OBJECTS_ZAP)
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a $(OPENAL) $(GLUT) $(THREADLIB) $(LFLAGS)


# We need one file in the master folder to be compiled to make this all work.  This
# will make sure the master is compiled, and will compile it if it is not.  We'll use
# the nophpbb option so that it will work on systems that have no phpbb integration
# going on, which, for most users, will be the case.  
#
# Note also that this rule needs to be below the bitfighter rule above.
#
../master/masterInterface.o:
	make -C ../master interface 

clean: 
	rm -f $(OBJECTS_ZAP) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_ZAP)

