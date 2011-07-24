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

OBJECTS_ZAP=\
   CTFGame.o\
   HTFGame.o\
   IniFile.o\
   loadoutZone.o\
   EditorObject.o\
   GeomUtils.o\
   Geometry.o\
   SimpleLine.o\
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
   Event.o\
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
   item.o\
   Joystick.o\
   JoystickRender.o\
   lineEditor.o\
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
   ScreenInfo.o\
   ScreenShooter.o\
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
   md5wrapper.o\
   md5.o\
   ../master/masterInterface.o\
   ../master/database.o\
   ../sqlite/sqlite3.o\
   ../Triangle/triangle.o\
   ../recast/RecastAlloc.o\
   ../recast/RecastMesh.o\
   ../clipper/clipper.o\
   BotNavMeshZone.o\
   OpenglUtils.o\
   Colors.o
# These last classes MUST be in this order for the linker to work


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
dedicated: GFX_LFLAGS=
dedicated: SFX_LFLAGS=
dedicated: EXEFILE=bitfighterd
dedicated: bitfighter


bitfighter: $(OBJECTS_ZAP) 
	$(CC) -o ../exe/$(EXEFILE) $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a ../lua/lua-vec/src/liblua.a ../alure/libalure.a $(SFX_LFLAGS) $(GFX_LFLAGS) $(THREADLIB) $(LFLAGS)


clean: 
	rm -f $(OBJECTS_ZAP) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_ZAP)

