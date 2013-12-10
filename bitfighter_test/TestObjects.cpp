//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "gtest/gtest.h"

#include "tnlNetObject.h"
#include "tnlGhostConnection.h"
#include "tnlPlatform.h"

#include "BfObject.h"
#include "gameType.h"
#include "ServerGame.h"
#include "ClientGame.h"
#include "SystemFunctions.h"

#include "GeomUtils.h"
#include "stringUtils.h"
#include "RenderUtils.h"

#include "TestUtils.h"

#include <string>
#include <cmath>

#ifdef TNL_OS_WIN32
#  include <windows.h>   // For ARRAYSIZE
#endif

namespace Zap {
using namespace std;

class ObjectTest : public testing::Test
{
   public:
      static void ProcessArg_test1(ServerGame *game, S32 argc, const char **argv)
      {
         for(S32 j = 1; j <= argc; j++)
         {
            game->cleanUp();
            game->processLevelLoadLine(j, 0, argv, game->getGameObjDatabase(), "some_non_existing_filename.level");
         }
      }
};


// For the most part, only care about it not crashing or segfault from random argv/argc...
TEST_F(ObjectTest, ProcessArguments) 
{
   ServerGame *game = newServerGame();
   const S32 argc = 40;
   const char *argv[argc] = {
      "This first string will be replaced by 'argv[0] =' below",
      "3", "4", "3", "6", "6", "4", "2", "6", "6", "3", 
      "4", "3", "4", "3", "6", "6", "4", "2", "6", "6", 
      "4", "3", "4", "3", "6", "6", "4", "2", "6", "6", 
      "4", "3", "4", "3", "6", "blah", "4", "2", "6" };

   U32 count = TNL::NetClassRep::getNetClassCount(NetClassGroupGame, NetClassTypeObject);
   for(U32 i = 0; i < count; i++)
   {
      NetClassRep *netClassRep = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i);
      argv[0] = netClassRep->getClassName();

      ProcessArg_test1(game, argc, argv);
   }

#define t(n) {argv[0] = n; ProcessArg_test1(game, argc, argv);}
   t("BarrierMaker");
   t("LevelName");
   t("LevelCredits");
   t("Script");
   t("MinPlayers");
   t("MaxPlayers");
   t("Team");
#undef t

   delete game;
}


TEST_F(ObjectTest, ServerClient)
{
   U32 count = TNL::NetClassRep::getNetClassCount(NetClassGroupGame, NetClassTypeObject);
   Vector<U8> isAdded;
   isAdded.resize(count);
   for(U32 i=0; i < count; i++)
      isAdded[i] = 0;

   static const U8 FlagServerGameAdded = 1;
   static const U8 FlagServerGameExist = 2;
   static const U8 FlagClientGameExist = 4;

   GamePair gamePair;
   ClientGame *clientGame = gamePair.client;
   ServerGame *serverGame = gamePair.server;
   GameConnection *gc_client = gamePair.client->getConnectionToServer();
   GameConnection *gc_server = dynamic_cast<GameConnection*>(gc_client->getRemoteConnectionObject());
   ASSERT_TRUE(gc_client != NULL);
   ASSERT_TRUE(gc_server != NULL);

   Vector<Point> geom;
   geom.push_back(Point(0,0));
   geom.push_back(Point(1,0));
   geom.push_back(Point(0,1));

   for(U32 i = 0; i < count; i++)
   {
      NetClassRep *netClassRep = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i);
      Object *obj = netClassRep->create();
      BfObject *bfobj = dynamic_cast<BfObject *>(obj);
      if(bfobj)
      {

         if(bfobj->mGeometry.getGeometry() == NULL)
         {
            printf("! %s mGeometry is NULL\n", bfobj->getClassName());
            delete obj;
         }
         else
         {
            //printf("+ %s\n", bfobj->getClassName());

            // First, add some geometry
            // is "GeomObject::" really needed to fix compile error of invalid number of arguments?
            bfobj->setExtent(Rect(0,0,1,1));
            bfobj->GeomObject::setGeom(geom);
            bfobj->addToGame(serverGame, serverGame->getGameObjDatabase());
            gc_server->objectLocalScopeAlways(bfobj); // Force it to scope to client, for testing.
            isAdded[i] |= FlagServerGameAdded;
         }
      }
      else
      {
         //printf("- %s not BfObject\n", obj->getClassName()); // Its only GameTypes that is not a BfObject...
         delete obj;
      }
   }

   gamePair.idle(1, 80);

   const Vector<DatabaseObject *> *objects = clientGame->getGameObjDatabase()->findObjects_fast();
   for(S32 i=0; i<objects->size(); i++)
   {
      BfObject *bfobj = dynamic_cast<BfObject *>((*objects)[i]);
      if(bfobj->getClassRep() != NULL)  // Barriers and some other objects might not be ghostable..
      {
         U32 id = bfobj->getClassId(NetClassGroupGame);
         isAdded[id] |= FlagClientGameExist;
      }
   }
   objects = serverGame->getGameObjDatabase()->findObjects_fast();
   for(S32 i=0; i<objects->size(); i++)
   {
      BfObject *bfobj = dynamic_cast<BfObject *>((*objects)[i]);
      if(bfobj->getClassRep() != NULL)
      {
         U32 id = bfobj->getClassId(NetClassGroupGame);
         isAdded[id] |= FlagServerGameExist;
      }
   }


   for(U32 i = 0; i < count; i++)
   {
      if((isAdded[i] & FlagServerGameAdded) && !(isAdded[i] & FlagServerGameExist))
      {
         const char *name = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i)->getClassName();
         printf("- %s was destroyed during game idle\n", name);
      }
      else if((isAdded[i] & FlagServerGameExist) && !(isAdded[i] & FlagClientGameExist))
      {
         const char *name = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i)->getClassName();
         printf("! %s is on ServerGame but not on ClientGame\n", name);
      }
      else if((isAdded[i] & FlagClientGameExist) && !(isAdded[i] & FlagServerGameExist))
      {
         const char *name = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i)->getClassName();
         printf("! %s is on ClientGame but not on ServerGame\n", name);
      }
   }
}
   
}; // namespace Zap
