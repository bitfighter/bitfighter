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
      static void process(ServerGame *game, S32 argc, const char **argv)
      {
         for(S32 j = 1; j <= argc; j++)
         {
            game->cleanUp();
            game->processLevelLoadLine(j, 0, argv, game->getGameObjDatabase(), "some_non_existing_filename.level");
         }
      }
};


/**
 * Ensures that processArguments with long, non-sensical argv does not segfault
 * for all registered NetClasses and a few special level directives.
 */
TEST_F(ObjectTest, ProcessArgumentsSanity) 
{
   ServerGame *game = newServerGame();
   const S32 argc = 40;
   const char *argv[argc] = {
      "This first string will be replaced by 'argv[0] =' below",
      "3", "4", "3", "6", "6", "4", "2", "6", "6", "3", 
      "4", "3", "4", "3", "6", "6", "4", "2", "6", "6", 
      "4", "3", "4", "3", "6", "6", "4", "2", "6", "6", 
      "4", "3", "4", "3", "6", "blah", "4", "2", "6" };

   // Substitute the class name for argv[0]
   U32 count = TNL::NetClassRep::getNetClassCount(NetClassGroupGame, NetClassTypeObject);
   for(U32 i = 0; i < count; i++)
   {
      NetClassRep *netClassRep = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i);
      argv[0] = netClassRep->getClassName();

      process(game, argc, argv);
   }

#define t(n) {argv[0] = n; process(game, argc, argv);}
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



struct GhostingRecord
{
  bool server;
  bool client;
};

/**
 * Instantiate and transmit one object of every registered type from a server to
 * a client. Ensures that the associated code paths do not crash.
 */
TEST_F(ObjectTest, GhostingSanity)
{
   // Track the transmission state of each object
   U32 classCount = TNL::NetClassRep::getNetClassCount(NetClassGroupGame, NetClassTypeObject);
   Vector<GhostingRecord> ghostingRecords;
   ghostingRecords.resize(classCount);
   for(U32 i=0; i < classCount; i++)
   {
      ghostingRecords[i].server = false;
      ghostingRecords[i].client = false;
   }

   // Create our pair of connected games
   GamePair gamePair;
   ClientGame *clientGame = gamePair.client;
   ServerGame *serverGame = gamePair.server;

   // Basic geometry to plug into polygon objects
   Vector<Point> geom;
   geom.push_back(Point(0,0));
   geom.push_back(Point(1,0));
   geom.push_back(Point(0,1));

   // Create one of each type of registered NetClass
   for(U32 i = 0; i < classCount; i++)
   {
      NetClassRep *netClassRep = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i);
      Object *obj = netClassRep->create();
      BfObject *bfobj = dynamic_cast<BfObject *>(obj);

      // Skip registered classes that aren't BfObjects (e.g. GameType) or don't have
      // a geometry at this point (ForceField)
      if(bfobj && bfobj->hasGeometry())
      {
         // First, add some geometry
         bfobj->setExtent(Rect(0,0,1,1));
         bfobj->GeomObject::setGeom(geom);
         bfobj->addToGame(serverGame, serverGame->getGameObjDatabase());
         ghostingRecords[i].server = true;
      }
      else
         delete bfobj;
   }

   // Idle to allow object replication
   gamePair.idle(10, 10);

   // Check whether the objects made it onto the client
   const Vector<DatabaseObject *> *objects = clientGame->getGameObjDatabase()->findObjects_fast();
   for(S32 i=0; i<objects->size(); i++)
   {
      BfObject *bfobj = dynamic_cast<BfObject *>((*objects)[i]);
      if(bfobj->getClassRep() != NULL)  // Barriers and some other objects might not be ghostable..
      {
         U32 id = bfobj->getClassId(NetClassGroupGame);
         ghostingRecords[id].client = true;
      }
   }

   for(U32 i = 0; i < classCount; i++)
   {
      NetClassRep *netClassRep = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i);

      // Expect that all objects on the server are on the client, with the
      // exception of PolyWalls and ForceFields, because these do not follow the
      // normal ghosting model.
      string className = netClassRep->getClassName();
      if(className != "PolyWall" && className != "ForceField")
      {
         EXPECT_EQ(ghostingRecords[i].server, ghostingRecords[i].client);
      }
   }
}



/**
 * Test some LUA commands to all objects
 */
TEST_F(ObjectTest, LuaSanity)
{
   // Track the transmission state of each object
   U32 classCount = TNL::NetClassRep::getNetClassCount(NetClassGroupGame, NetClassTypeObject);

   // Create our pair of connected games
   GamePair gamePair;
   ClientGame *clientGame = gamePair.client;
   ServerGame *serverGame = gamePair.server;

   // Basic geometry to plug into polygon objects
   Vector<Point> geom;
   geom.push_back(Point(0,0));
   geom.push_back(Point(1,0));
   geom.push_back(Point(0,1));

   lua_State *L = lua_open();

   // Create one of each type of registered NetClass
   for(U32 i = 0; i < classCount; i++)
   {
      NetClassRep *netClassRep = TNL::NetClassRep::getClass(NetClassGroupGame, NetClassTypeObject, i);
      Object *obj = netClassRep->create();
      BfObject *bfobj = dynamic_cast<BfObject *>(obj);

      // Skip registered classes that aren't BfObjects (e.g. GameType) or don't have
      // a geometry at this point (ForceField)
      if(bfobj && bfobj->hasGeometry())
      {
         // First, add some geometry
         bfobj->setExtent(Rect(0,0,1,1));
         bfobj->GeomObject::setGeom(geom);

         // LUA testing
         lua_pushinteger(L, 1);
         bfobj->lua_setTeam(L);
         lua_pop(L, 1);
         lua_pushinteger(L, -2);
         bfobj->lua_setTeam(L);
         lua_pop(L, 1);
         lua_pushvec(L, 2.3f, 4.3f);
         bfobj->lua_setPos(L);
         lua_pop(L, 1);

         bfobj->addToGame(serverGame, serverGame->getGameObjDatabase());
      }
      else
         delete bfobj;
   }


   lua_close(L);
}

   
}; // namespace Zap
