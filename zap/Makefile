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
#######################################

CC=g++ -g  -I../tnl -I../glut -I../openal -DTNL_ENABLE_LOGGING #-O2
THREADLIB= -lpthread
GLUT=-lGL -lGLU -lglut
INPUT=linuxInput.o 

OBJECTS_ZAP=\
   CTFGame.o\
   HTFGame.o\
   IniFile.o\
   LoadoutZone.o\
   SweptEllipsoid.o\
   UI.o\
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
   UIMenus.o\
   UINameEntry.o\
   UIQueryServers.o\
   UIGameParameters.o\
   barrier.o\
   config.o\
   controlObjectConnection.o\
   engineeredObjects.o\
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
   huntersGame.o\
   input.o\
   item.o\
   $(INPUT)\
   loadoutSelect.o\
   luaGameInfo.o\
   luaLevelGenerator.o\
   luaObject.o\
   luaUtil.o\
   main.o\
   masterConnection.o\
   move.o\
   moveObject.o\
   projectile.o\
   quickChat.o\
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
   ../master/masterInterface.o\

CFLAGS=
DFLAGS=
EXEFILE=bitfighter
OPENAL=-lopenal -lalut
LUALIB=-llua5.1
WFLAGS=



.c.o:
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) $<

.cpp.o : 
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) $<


default: ../exe/bitfighter

bitfighter: ../exe/bitfighter

bitfightered: ../exe/bitfightered

dedicated: CFLAGS=-DZAP_DEDICATED 
dedicated: GLUT=
dedicated: OPENAL=
dedicated: EXEFILE=bitfighterd
dedicated: ../exe/bitfighter


../exe/bitfighter: $(OBJECTS_ZAP)
# Avoid errors with mingw by removing gch files with mingw 
#  rm -f *.gch    
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a  $(OPENAL) $(GLUT) $(THREADLIB) $(LUALIB) -lstdc++  -lm

../master/masterInterface.o:
	make -C ../master

clean:
	rm -f $(OBJECTS_ZAP) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_ZAP)

