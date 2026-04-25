LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
ZAP_PATH := ../zap

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
$(LOCAL_PATH)/$(ZAP_PATH)/../tnl \
$(LOCAL_PATH)/$(ZAP_PATH)/../recast \
$(LOCAL_PATH)/$(ZAP_PATH)/../boost \
$(LOCAL_PATH)/$(ZAP_PATH)/../sqlite \

# using -DTNL_DEBUG hangs on start-up
LOCAL_CFLAGS := -DBF_NO_AUDIO -Dfdatasync=fsync

# Shared sources
SHARED_SOURCES := \
$(ZAP_PATH)/BanList.cpp \
$(ZAP_PATH)/barrier.cpp \
$(ZAP_PATH)/BfObject.cpp \
$(ZAP_PATH)/BotNavMeshZone.cpp \
$(ZAP_PATH)/ChatCheck.cpp \
$(ZAP_PATH)/ClientInfo.cpp \
$(ZAP_PATH)/Color.cpp \
$(ZAP_PATH)/config.cpp \
$(ZAP_PATH)/Console.cpp \
$(ZAP_PATH)/controlObjectConnection.cpp \
$(ZAP_PATH)/CoreGame.cpp \
$(ZAP_PATH)/CTFGame.cpp \
$(ZAP_PATH)/dataConnection.cpp \
$(ZAP_PATH)/EditorPlugin.cpp \
$(ZAP_PATH)/EngineeredItem.cpp \
$(ZAP_PATH)/EventManager.cpp \
$(ZAP_PATH)/flagItem.cpp \
$(ZAP_PATH)/game.cpp \
$(ZAP_PATH)/gameConnection.cpp \
$(ZAP_PATH)/gameLoader.cpp \
$(ZAP_PATH)/gameNetInterface.cpp \
$(ZAP_PATH)/GameSettings.cpp \
$(ZAP_PATH)/gameStats.cpp \
$(ZAP_PATH)/gameType.cpp \
$(ZAP_PATH)/gameWeapons.cpp \
$(ZAP_PATH)/Geometry.cpp \
$(ZAP_PATH)/GeomObject.cpp \
$(ZAP_PATH)/GeomUtils.cpp \
$(ZAP_PATH)/goalZone.cpp \
$(ZAP_PATH)/gridDB.cpp \
$(ZAP_PATH)/HTFGame.cpp \
$(ZAP_PATH)/HttpRequest.cpp \
$(ZAP_PATH)/IniFile.cpp \
$(ZAP_PATH)/InputCode.cpp \
$(ZAP_PATH)/item.cpp \
$(ZAP_PATH)/LineItem.cpp \
$(ZAP_PATH)/LoadoutTracker.cpp \
$(ZAP_PATH)/loadoutZone.cpp \
$(ZAP_PATH)/LuaBase.cpp \
$(ZAP_PATH)/luaGameInfo.cpp \
$(ZAP_PATH)/luaLevelGenerator.cpp \
$(ZAP_PATH)/LuaScriptRunner.cpp \
$(ZAP_PATH)/masterConnection.cpp \
$(ZAP_PATH)/MathUtils.cpp \
$(ZAP_PATH)/md5wrapper.cpp \
$(ZAP_PATH)/move.cpp \
$(ZAP_PATH)/moveObject.cpp \
$(ZAP_PATH)/NexusGame.cpp \
$(ZAP_PATH)/PickupItem.cpp \
$(ZAP_PATH)/playerInfo.cpp \
$(ZAP_PATH)/Point.cpp \
$(ZAP_PATH)/PointObject.cpp \
$(ZAP_PATH)/polygon.cpp \
$(ZAP_PATH)/projectile.cpp \
$(ZAP_PATH)/rabbitGame.cpp \
$(ZAP_PATH)/Rect.cpp \
$(ZAP_PATH)/retrieveGame.cpp \
$(ZAP_PATH)/robot.cpp \
$(ZAP_PATH)/ScreenInfo.cpp \
$(ZAP_PATH)/ServerGame.cpp \
$(ZAP_PATH)/ship.cpp \
$(ZAP_PATH)/shipItems.cpp \
$(ZAP_PATH)/SimpleLine.cpp \
$(ZAP_PATH)/SlipZone.cpp \
$(ZAP_PATH)/soccerGame.cpp \
$(ZAP_PATH)/SoundEffect.cpp \
$(ZAP_PATH)/SoundSystem.cpp \
$(ZAP_PATH)/Spawn.cpp \
$(ZAP_PATH)/speedZone.cpp \
$(ZAP_PATH)/statistics.cpp \
$(ZAP_PATH)/stringUtils.cpp \
$(ZAP_PATH)/SystemFunctions.cpp \
$(ZAP_PATH)/teamInfo.cpp \
$(ZAP_PATH)/teleporter.cpp \
$(ZAP_PATH)/textItem.cpp \
$(ZAP_PATH)/Timer.cpp \
$(ZAP_PATH)/WallSegmentManager.cpp \
$(ZAP_PATH)/WeaponInfo.cpp \
$(ZAP_PATH)/Zone.cpp \
$(ZAP_PATH)/zoneControlGame.cpp \
$(ZAP_PATH)/../clipper/clipper.cpp \
$(ZAP_PATH)/../master/database.cpp \
$(ZAP_PATH)/../master/masterInterface.cpp \
$(ZAP_PATH)/../recast/RecastAlloc.cpp \
$(ZAP_PATH)/../recast/RecastMesh.cpp \
$(ZAP_PATH)/../sqlite/sqlite3.c \
$(ZAP_PATH)/../poly2tri/common/shapes.cc \
$(ZAP_PATH)/../poly2tri/sweep/advancing_front.cc \
$(ZAP_PATH)/../poly2tri/sweep/cdt.cc \
$(ZAP_PATH)/../poly2tri/sweep/sweep.cc \
$(ZAP_PATH)/../poly2tri/sweep/sweep_context.cc \

# Client sources
CLIENT_SOURCES := \
$(ZAP_PATH)/AToBScroller.cpp \
$(ZAP_PATH)/ChatCommands.cpp \
$(ZAP_PATH)/ChatHelper.cpp \
$(ZAP_PATH)/ClientGame.cpp \
$(ZAP_PATH)/Cursor.cpp \
$(ZAP_PATH)/EditorAttributeMenuItemBuilder.cpp \
$(ZAP_PATH)/EditorTeam.cpp \
$(ZAP_PATH)/EnergyGaugeRenderer.cpp \
$(ZAP_PATH)/engineerHelper.cpp \
$(ZAP_PATH)/Event.cpp \
$(ZAP_PATH)/FontManager.cpp \
$(ZAP_PATH)/FpsRenderer.cpp \
$(ZAP_PATH)/gameObjectRender.cpp \
$(ZAP_PATH)/HelpItemManager.cpp \
$(ZAP_PATH)/HelperManager.cpp \
$(ZAP_PATH)/helperMenu.cpp \
$(ZAP_PATH)/Joystick.cpp \
$(ZAP_PATH)/JoystickRender.cpp \
$(ZAP_PATH)/LevelDatabaseDownloadThread.cpp \
$(ZAP_PATH)/LevelDatabaseRateThread.cpp \
$(ZAP_PATH)/LevelDatabaseUploadThread.cpp \
$(ZAP_PATH)/lineEditor.cpp \
$(ZAP_PATH)/LoadoutIndicator.cpp \
$(ZAP_PATH)/loadoutHelper.cpp \
$(ZAP_PATH)/OpenglUtils.cpp \
$(ZAP_PATH)/quickChatHelper.cpp \
$(ZAP_PATH)/RenderUtils.cpp \
$(ZAP_PATH)/ScissorsManager.cpp \
$(ZAP_PATH)/ShipShape.cpp \
$(ZAP_PATH)/SlideOutWidget.cpp \
$(ZAP_PATH)/sparkManager.cpp \
$(ZAP_PATH)/SymbolShape.cpp \
$(ZAP_PATH)/TeamShuffleHelper.cpp \
$(ZAP_PATH)/TimeLeftRenderer.cpp \
$(ZAP_PATH)/UI.cpp \
$(ZAP_PATH)/UIAbstractInstructions.cpp \
$(ZAP_PATH)/UIChat.cpp \
$(ZAP_PATH)/UICredits.cpp \
$(ZAP_PATH)/UIDiagnostics.cpp \
$(ZAP_PATH)/UIEditor.cpp \
$(ZAP_PATH)/UIEditorInstructions.cpp \
$(ZAP_PATH)/UIEditorMenus.cpp \
$(ZAP_PATH)/UIErrorMessage.cpp \
$(ZAP_PATH)/UIGame.cpp \
$(ZAP_PATH)/UIGameParameters.cpp \
$(ZAP_PATH)/UIHighScores.cpp \
$(ZAP_PATH)/UIInstructions.cpp \
$(ZAP_PATH)/UIKeyDefMenu.cpp \
$(ZAP_PATH)/UILevelInfoDisplayer.cpp \
$(ZAP_PATH)/UIManager.cpp \
$(ZAP_PATH)/UIMenuItems.cpp \
$(ZAP_PATH)/UIMenus.cpp \
$(ZAP_PATH)/UIMessage.cpp \
$(ZAP_PATH)/UINameEntry.cpp \
$(ZAP_PATH)/UIQueryServers.cpp \
$(ZAP_PATH)/UITeamDefMenu.cpp \
$(ZAP_PATH)/UIYesNo.cpp \
$(ZAP_PATH)/VideoSystem.cpp \
$(ZAP_PATH)/voiceCodec.cpp \
$(ZAP_PATH)/../fontstash/stb_truetype.c \
$(ZAP_PATH)/../fontstash/fontstash.c \

# Add your application source files here...
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
$(SHARED_SOURCES) \
$(CLIENT_SOURCES) \
$(ZAP_PATH)/main.cpp \

# Classes not compiled for Android
#$(ZAP_PATH)/ScreenShooter.cpp \
#$(ZAP_PATH)/oglconsole.cpp \

LOCAL_SHARED_LIBRARIES := tnl luavec tomcrypt SDL2

LOCAL_LDLIBS := -llog -lGLESv1_CM

include $(BUILD_SHARED_LIBRARY)
