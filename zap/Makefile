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
CC=gcc $(DEBUG) $(INCLUDES) -Wall $(OPTIMIZE)
THREADLIB= -lpthread
EXEFILE=bitfighter


# Mingw in windows
ifeq ($(findstring Windows, $(OS)), Windows)
CC += -I../libsdl/SDL2 -I../libpng -I../zlib -DALURE_STATIC_LIBRARY
LFLAGS=-static -lstdc++ -L../lib -lwsock32 -lwinmm
SFX_LFLAGS=-lopenal32 -llibspeex -llibvorbis
GFX_LFLAGS=-lSDL2 -lopengl32 -llibpng14 -llibmodplug
else
CC +=  -I../libsdl/SDL

# sound linker flags
SFX_LFLAGS=-lopenal -lspeex -lvorbis -lmodplug

# graphics linker flags
PNG_LFLAGS:=$(shell pkg-config --libs libpng)
GFX_LFLAGS=-lSDL -lGL -lX11 $(PNG_LFLAGS)

# Other needed linker flags
LFLAGS=-lstdc++ -lm -ldl

endif

# Other compiler flags
CFLAGS=
DFLAGS=
WFLAGS=


OBJECTS_SHARED=\
   BanList.o\
   barrier.o\
   BotNavMeshZone.o\
   ChatCheck.o\
   ClientInfo.o\
   Color.o\
   config.o\
   controlObjectConnection.o\
   CoreGame.o\
   CTFGame.o\
   dataConnection.o\
   EditorObject.o\
   EditorPlugin.o\
   EngineeredItem.o\
   EventManager.o\
   flagItem.o\
   game.o\
   gameConnection.o\
   gameLoader.o\
   gameNetInterface.o\
   BfObject.o\
   GameSettings.o\
   gameStats.o\
   gameType.o\
   gameWeapons.o\
   Geometry.o\
   GeomObject.o\
   GeomUtils.o\
   goalZone.o\
   gridDB.o\
   HTFGame.o\
   IniFile.o\
   InputCode.o\
   item.o\
   lineEditor.o\
   LineItem.o\
   loadoutZone.o\
   LuaBase.o\
   luaGameInfo.o\
   luaLevelGenerator.o\
   LuaScriptRunner.o\
   main.o\
   masterConnection.o\
   MathUtils.o\
   md5wrapper.o\
   move.o\
   moveObject.o\
   NexusGame.o\
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
   ServerGame.o\
   ship.o\
   shipItems.o\
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
   WallSegmentManager.o\
   WeaponInfo.o\
   Zone.o\
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
   Console.o\
   Cursor.o\
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
   RenderUtils.o\
   ScreenShooter.o\
   ShipShape.o\
   sparkManager.o\
   TeamShuffleHelper.o\
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
   UIHighScores.o\
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
   VideoSystem.o\
   voiceCodec.o\
   $(OBJECTS_SHARED)


OBJECTS_DEDICATED=\
   $(OBJECTS_SHARED)


OBJECTS_FASTBUILD=\
	build1.o\
	build2.o\
	build3.o\
	build4.o\
	../clipper/clipper.cpp\
	../master/database.cpp\
	../master/masterInterface.cpp\
	oglconsole.c\
	../recast/RecastAlloc.cpp\
	../recast/RecastMesh.cpp\
	../sqlite/sqlite3.c\
	../Triangle/triangle.c

.c.o:
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) -o $@ $<

.cc.o: 
	$(CC) $(DFLAGS) $(WFLAGS) -c $(CFLAGS) -o $@ $<

.cpp.o: 
	$(CC) -Wno-c++0x-compat $(DFLAGS) $(WFLAGS) -c $(CFLAGS) -o $@ $<


default: release

debug: OPTIMIZE=
debug: bitfighter

release: DEBUG=
release: bitfighter
fastbuild: DEBUG=

dedicated: DEBUG=
dedicated: CFLAGS=-DZAP_DEDICATED 
dedicated: EXEFILE=bitfighterd
dedicated: dedicated_server

dedicated_debug: OPTIMIZE=
dedicated_debug: CFLAGS=-DZAP_DEDICATED 
dedicated_debug: EXEFILE=bitfighterd
dedicated_debug: dedicated_server

## -rdynamic is used for backtrace_symbols in main.cpp, when using -DUSE_EXCEPTION_BACKTRACE (for main.cpp)
## dedicated_server: LFLAGS += -rdynamic
## dedicated_server: CFLAGS += -DUSE_EXCEPTION_BACKTRACE

bitfighter: $(OBJECTS_CLIENT) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_CLIENT) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a ../alure/libalure.a $(SFX_LFLAGS) $(GFX_LFLAGS) $(THREADLIB) $(LFLAGS)

dedicated_server: $(OBJECTS_DEDICATED) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_DEDICATED) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a $(THREADLIB) $(LFLAGS)

fastbuild: $(OBJECTS_FASTBUILD) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_FASTBUILD) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a ../alure/libalure.a $(SFX_LFLAGS) $(GFX_LFLAGS) $(THREADLIB) $(LFLAGS)

clean: 
	rm -f $(OBJECTS_CLIENT) $(OBJECTS_CLIENT) $(OBJECTS_DEDICATED) $(OBJECTS_SHARED) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_CLIENT) $(OBJECTS_DEDICATED) $(OBJECTS_SHARED)

