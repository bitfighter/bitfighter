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

OPTIMIZE=-O2
DEBUG=-g
INCLUDES=-I../tnl -I../glut -I../openal -I../Triangle -I../recast -I../clipper -I../alure -I../libspeex -I../boost
CC=gcc $(DEBUG) $(INCLUDES) -DTNL_ENABLE_LOGGING -W $(OPTIMIZE)
THREADLIB= -lpthread
GLUT=-lGL -lGLU -lglut
INPUT=linuxInput.o 

OBJECTS_ZAP=\
   CTFGame.o\
   HTFGame.o\
   IniFile.o\
   loadoutZone.o\
   GeomUtils.o\
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
   Color.o\
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
   gameStats.o\
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
   Point.o\
   polygon.o\
   projectile.o\
   quickChatHelper.o\
   rabbitGame.o\
   Rect.o\
   retrieveGame.o\
   robot.o\
   screenShooter.o\
   SoundSystem.o\
   SoundEffect.o\
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
   ../master/masterInterface.o\
   ../master/database.o\
   ../sqlite/sqlite3.o\
   ../Triangle/triangle.o\
   ../recast/RecastAlloc.o\
   ../recast/RecastMesh.o\
   ../clipper/clipper.o


CFLAGS=
DFLAGS=
EXEFILE=bitfighter
OPENAL=-lopenal -lspeex
WFLAGS=
LFLAGS=-lstdc++ -lm -ldl -lX11

.c.o:
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) -o $@ $<

.cc.o: 
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) -o $@ $<

.cpp.o: 
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) -o $@ $<


default: release

debug: OPTIMIZE=
debug: bitfighter

release: DEBUG=
release: bitfighter

dedicated: DEBUG=
dedicated: CFLAGS=-DZAP_DEDICATED 
dedicated: LFLAGS=-lstdc++ -lm -ldl
dedicated: GLUT=
dedicated: OPENAL=
dedicated: EXEFILE=bitfighterd
dedicated: bitfighter


bitfighter: $(OBJECTS_ZAP) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a ../alure/libalure.a $(OPENAL) $(GLUT) $(THREADLIB) $(LFLAGS)


clean: 
	rm -f $(OBJECTS_ZAP) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_ZAP)

