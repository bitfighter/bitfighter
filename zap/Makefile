# Bitfighter Makefile

# 
# Configuration
#

#######################################
# For Linux/Mac users, uncomment the following line, and comment out the Windows line below
CC=g++ -g  -I../tnl -I../glut -I../openal -DTNL_DEBUG -DTNL_ENABLE_LOGGING #-O2
threadLib= -lpthread
glut=-lGL -lGLU -lglut
linuxInput=linuxInput.o
#######################################
# For Windows users, uncomment the following line, and comment out the line above
#CC=g++ -g -I../tnl -I../glut -I../openal -DTNL_DEBUG -DTNL_ENABLE_LOGGING -DWIN32 -D_STDCALL_SUPPORTED #-O2
#threadLib=
#glut=-lglut32
#linuxInput=winJoystick.o
#######################################


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
   $(linuxInput)\
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

#CFLAGS=
CFLAGS=-DZAP_DEDICATED

.c.o:
	$(CC) -c $(CFLAGS) $<

.cpp.o : 
	$(CC) -c $(CFLAGS) $<


default: ../exe/bitfighter

bitfighter: ../exe/bitfighter

bitfightered: ../exe/bitfightered


../exe/bitfighter: $(OBJECTS_ZAP)
# Avoid errors with mingw by removing gch files 
#	rm -f *.gch		


#	$(CC) -o ../exe/bitfighter $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a  ../openal/linux/libopenal.a -s -lglut32 -mwindows
#	$(CC) -o ../exe/bitfighter $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a  ../openal/linux/libopenal.a $(glut) $(threadLib) -llua5.1 -lstdc++  -lm
#CENTOS:
	$(CC) -o ../exe/bitfighter $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a  ../openal/linux/libopenal.a $(threadLib) /usr/lib/liblua.a -lstdc++  -lm
#../exe/bitfightered: CFLAGS=-DZAP_DEDICATED
#../exe/bitfightered: $(OBJECTS_ZAP)
#	$(CC) -o ../exe/bitfightered $(OBJECTS_ZAP) ../tnl/libtnl.a ../libtomcrypt/libtomcrypt.a  $(threadLib) -lstdc++ -lm

../master/masterInterface.o:
	make -C ../master

clean:
	rm -f $(OBJECTS_ZAP) ../exe/bitfighter ../exe/bitfightered

cleano:
	rm -f $(OBJECTS_ZAP)
