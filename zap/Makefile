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
#     WFLAGS="-DWIN32 -D_STDCALL_SUPPORTED" THREADLIB=
# to the make command line to have any hope of getting it to work!  :-)
#
#
#######################################

OPTIMIZE=-O2
DEBUG=-g -DTNL_DEBUG
INCLUDES=-I../tnl -I../openal -I../Triangle -I../recast -I../clipper -I../alure -I../libspeex -I../boost
CC=gcc $(DEBUG) $(INCLUDES) -DTNL_ENABLE_LOGGING -W $(OPTIMIZE)
THREADLIB= -lpthread
EXEFILE=bitfighter

# sound linker flags
SFX_LFLAGS=-lopenal -lspeex -lvorbis

# graphics linker flags
PNG_LFLAGS:=$(shell pkg-config --libs libpng)
GFX_LFLAGS=-lSDL -lGL $(PNG_LFLAGS)

# Other needed linker flags
LFLAGS=-lstdc++ -lm -ldl

# Other compiler flags
CFLAGS=
DFLAGS=
WFLAGS=


OBJECTS_SHARED=\
   barrier.o\
   BotNavMeshZone.o\
   Color.o\
   config.o\
   controlObjectConnection.o\
   CTFGame.o\
   dataConnection.o\
   EngineeredItem.o\
   flagItem.o\
   game.o\
   gameConnection.o\
   gameLoader.o\
   gameNetInterface.o\
   gameObject.o\
   GameSettings.o\
   gameStats.o\
   gameType.o\
   gameWeapons.o\
   Geometry.o\
   GeomUtils.o\
   goalZone.o\
   gridDB.o\
   HTFGame.o\
   huntersGame.o\
   IniFile.o\
   item.o\
   keyCode.o\
   lineEditor.o\
   loadoutZone.o\
   luaGameInfo.o\
   luaLevelGenerator.o\
   luaObject.o\
   luaUtil.o\
   main.o\
   masterConnection.o\
   md5.o\
   md5wrapper.o\
   move.o\
   moveObject.o\
   PickupItem.o\
   playerInfo.o\
   Point.o\
   polygon.o\
   projectile.o\
   rabbitGame.o\
   Rect.o\
   retrieveGame.o\
   robot.o\
   ScreenInfo.o\
   ship.o\
   SimpleLine.o\
   SlipZone.o\
   soccerGame.o\
   SoundEffect.o\
   SoundSystem.o\
   Spawn.o\
   speedZone.o\
   statistics.o\
   stringUtils.o\
   teamInfo.o\
   teleporter.o\
   textItem.o\
   Timer.o\
   zoneControlGame.o\
   ../clipper/clipper.o\
   ../master/masterInterface.o\
   ../master/database.o\
   ../recast/RecastAlloc.o\
   ../recast/RecastMesh.o\
   ../sqlite/sqlite3.o\
   ../Triangle/triangle.o


OBJECTS_CLIENT=\
   ClientGame.o\
   EditorObject.o\
   engineerHelper.o\
   Event.o\
   gameObjectRender.o\
   helperMenu.o\
   Joystick.o\
   JoystickRender.o\
   loadoutHelper.o\
   oglconsole.o\
   OpenglUtils.o\
   quickChatHelper.o\
   ScreenShooter.o\
   sparkManager.o\
   UI.o\
   UIAbstractInstructions.o\
   UIChat.o\
   UICredits.o\
   UIDiagnostics.o\
   UIEditor.o\
   UIEditorInstructions.o\
   UIEditorMenus.o\
   UIErrorMessage.o\
   UIYesNo.o\
   UIGame.o\
   UIInstructions.o\
   UIKeyDefMenu.o\
   UITeamDefMenu.o\
   UIManager.o\
   UIMenuItems.o\
   UIMenus.o\
   UIMessage.o\
   UINameEntry.o\
   UIQueryServers.o\
   UIGameParameters.o\
   voiceCodec.o\
   $(OBJECTS_SHARED)


OBJECTS_DEDICATED=\
   $(OBJECTS_SHARED)


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
dedicated: EXEFILE=bitfighterd
dedicated: dedicated_server

dedicated_debug: OPTIMIZE=
dedicated_debug: CFLAGS=-DZAP_DEDICATED 
dedicated_debug: EXEFILE=bitfighterd
dedicated_debug: dedicated_server


bitfighter: $(OBJECTS_CLIENT) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_CLIENT) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a ../alure/libalure.a $(SFX_LFLAGS) $(GFX_LFLAGS) $(THREADLIB) $(LFLAGS)

dedicated_server: $(OBJECTS_DEDICATED) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_DEDICATED) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a $(THREADLIB) $(LFLAGS)

clean: 
	rm -f $(OBJECTS_CLIENT) $(OBJECTS_CLIENT) $(OBJECTS_DEDICATED) $(OBJECTS_SHARED) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_CLIENT) $(OBJECTS_DEDICATED) $(OBJECTS_SHARED)

