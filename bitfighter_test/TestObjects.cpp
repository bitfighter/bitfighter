//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Bitfighter Tests

#define BF_TEST

#include "gtest/gtest.h"

#include "BfObject.h"
#include "gameType.h"
#include "ServerGame.h"
#include "ClientGame.h"
#include "Level.h"
#include "SystemFunctions.h"
#include "LevelFilesForTesting.h"

#include "LuaScriptRunner.h"

#include "Colors.h"
#include "GeomUtils.h"
#include "stringUtils.h"
#include "RenderUtils.h"

#include "TestUtils.h"

#include "tnlNetObject.h"
#include "tnlGhostConnection.h"
#include "tnlPlatform.h"


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
      // argv will be a single header directive followed by a bunch of garbage
      static void process(ServerGame *game, S32 argc, const char **argv)
      {
         string err;

         for(S32 j = 1; j <= argc; j++)
         {
            Level level;
            level.processLevelLoadLine(j, 0, argv, err);
         }
      }
};


void testObjectTransmission(S32 objTypeNumber, ServerGame *serverGame, S32 severCount,
                                               ClientGame *blue,       S32 blueCount,
                                               ClientGame *red,        S32 redCount)
{
   Vector<DatabaseObject *> fillVector;

   // Repair should be everywhere
   serverGame->getLevel()->findObjects(objTypeNumber, fillVector);
   ASSERT_EQ(severCount, fillVector.size()) << "Server";
   fillVector.clear();
   blue->getLevel()->findObjects(objTypeNumber, fillVector);
   EXPECT_EQ(blueCount, fillVector.size()) << "Blue client";
   fillVector.clear();
   red->getLevel()->findObjects(objTypeNumber, fillVector);
   EXPECT_EQ(redCount, fillVector.size()) << "Red client";
   fillVector.clear();
}


void checkObjects(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   {
      SCOPED_TRACE("RepairItem");
      testObjectTransmission(RepairItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 1, red, 0);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 1);
   }
   {
      SCOPED_TRACE("Zone");
      testObjectTransmission(ZoneTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


void checkObjectsWhenBothPlayersAreOnRed(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("BothPlayersAreOnRed");
   {
      SCOPED_TRACE("RepairItem");
      testObjectTransmission(RepairItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Zone");
      testObjectTransmission(ZoneTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


void checkObjectsWhenNeutral(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("Objects are Neutral");
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 1, red, 1);
   }
}

void checkObjectsWhenHostile(ServerGame *serverGame, ClientGame *blue, ClientGame *red)
{
   SCOPED_TRACE("Objects are Hostile");
   {
      SCOPED_TRACE("Blue Line");
      testObjectTransmission(LineTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
   {
      SCOPED_TRACE("Red Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 0);
   }
}


TEST_F(ObjectTest, TestItemPropagation)
{
   // Create a GamePair using our text item code, with 2 clients; one will be red, the other blue.
   // The test will confirm which players get the red text item, the blue line, and the zone object.
   string levelCode = getLevelCodeForItemPropagationTests();

   GamePair gamePair(levelCode, 2);

   // First, ensure we have two players, one red, one blue
   ServerGame *serverGame = gamePair.server;

   ASSERT_EQ(2, serverGame->getPlayerCount());
   ASSERT_EQ(Colors::blue.toHexString(), serverGame->getTeamColor(0).toHexString());
   ASSERT_EQ(Colors::red.toHexString(), serverGame->getTeamColor(1).toHexString());

   // Do the following rigamarole to break dependency on assumption client0 is blue and client1 is red
   ClientGame *client0 = gamePair.getClient(0);
   ClientGame *client1 = gamePair.getClient(1);

   ClientGame *blue = (serverGame->getTeamColor(client0->getLocalRemoteClientInfo()->getTeamIndex()).toHexString() == 
                              Colors::blue.toHexString()) ? client0 : client1;
      
   ClientGame *red = (serverGame->getTeamColor(client0->getLocalRemoteClientInfo()->getTeamIndex()).toHexString() ==
                              Colors::red.toHexString()) ? client0 : client1;

   ASSERT_EQ(Colors::blue.toHexString(), serverGame->getTeamColor(blue->getLocalRemoteClientInfo()->getTeamIndex()).toHexString());
   ASSERT_EQ(Colors::red.toHexString(), serverGame->getTeamColor(red->getLocalRemoteClientInfo()->getTeamIndex()).toHexString());

   ASSERT_FALSE(serverGame->getClientInfos()->get(0)->getConnection()->mInCommanderMap);
   ASSERT_FALSE(serverGame->getClientInfos()->get(1)->getConnection()->mInCommanderMap);

   // Now that we know which client is which, we can check to see which objects are available where

   {
      SCOPED_TRACE("Not in CommandersMap");
      checkObjects(serverGame, blue, red);
   }

   // Turn on cmdrs map... should not affect results
   red->setUsingCommandersMap(true);
   blue->setUsingCommandersMap(true);

   gamePair.idle(10, 5);     // Let things settle

   ASSERT_TRUE(serverGame->getClientInfos()->get(0)->getConnection()->mInCommanderMap);
   ASSERT_TRUE(serverGame->getClientInfos()->get(1)->getConnection()->mInCommanderMap);

   {
      SCOPED_TRACE("In CommandersMap");
      checkObjects(serverGame, blue, red);
   }

   Vector<DatabaseObject *> fillVector;

   // Change the textitem from red to blue
   serverGame->getLevel()->findObjects(TextItemTypeNumber, fillVector);
   ASSERT_EQ(1, fillVector.size());
   BfObject *obj = static_cast<BfObject *>(fillVector[0]);
   obj->setTeam(0);     // Blue team

   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Blue Text");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 1, red, 0);
   }
   obj->setTeam(1);          // Back to red

   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Red Text (after revert)");
      testObjectTransmission(TextItemTypeNumber, serverGame, 1, blue, 0, red, 1);
   }

   // At the moment, chaning a player's team won't delete objects that are no longer in scope for team reasons,
   // such as TextItems and LineItems until those items have been updated somehow.  Which they never are.  Bummer.
   //
   //// Now change the player's team from blue to red
   //blue->changeOwnTeam(1);
   //EXPECT_EQ(0, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 0 (change hasn't propagated yet)";
   //gamePair.idle(10, 5);     // Let things settle
   //EXPECT_EQ(1, blue->getLocalRemoteClientInfo()->getTeamIndex()) << "Expect this client to be on team 1 (change should have propagated)";

   //EXPECT_EQ(1, serverGame->getClientInfos()->get(0)->getTeamIndex());
   //EXPECT_EQ(1, serverGame->getClientInfos()->get(1)->getTeamIndex());

   //checkObjectsWhenBothPlayersAreOnRed(serverGame, blue, red);

   // Make items neutral and see if they propagate properly
   fillVector.clear();
   serverGame->getLevel()->findObjects(TextItemTypeNumber, fillVector);
   serverGame->getLevel()->findObjects(LineTypeNumber, fillVector);
   ASSERT_EQ(2, fillVector.size());
   for(S32 i = 0; i < fillVector.size(); i++)
      static_cast<BfObject *>(fillVector[i])->setTeam(TEAM_NEUTRAL);
   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Neutral!!");
      checkObjectsWhenNeutral(serverGame, blue, red);
   }

   for(S32 i = 0; i < fillVector.size(); i++)
      static_cast<BfObject *>(fillVector[i])->setTeam(TEAM_HOSTILE);
   gamePair.idle(10, 5);     // Let things settle
   {
      SCOPED_TRACE("Hostile!!");
      checkObjectsWhenHostile(serverGame, blue, red);
   }
}


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
   for(U32 i = 0; i < classCount; i++)
   {
      ghostingRecords[i].server = false;
      ghostingRecords[i].client = false;
   }

   // Create a pair of connected games
   GamePair gamePair;
   ClientGame *clientGame = gamePair.getClient(0);
   ServerGame *serverGame = gamePair.server;

   // Basic geometry to plug into polygon objects
   Vector<Point> geom;
   geom.push_back(Point(0,0));
   geom.push_back(Point(1,0));
   geom.push_back(Point(0,1));

   Vector<Point> geom_speedZone;
   geom_speedZone.push_back(Point(400,0));
   geom_speedZone.push_back(Point(400,1));

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
         if(bfobj->getObjectTypeNumber() == SpeedZoneTypeNumber)
         {
            bfobj->GeomObject::setGeom(geom_speedZone);
            bfobj->onGeomChanged();
         }
         else
            bfobj->GeomObject::setGeom(geom);
         bfobj->addToGame(serverGame, serverGame->getLevel());

         ghostingRecords[i].server = true;
      }
      else
         delete bfobj;
   }

   // Idle to allow object replication
   gamePair.idle(10, 10);

   // Check whether the objects created on the server made it onto the client
   const Vector<DatabaseObject *> *objects = clientGame->getLevel()->findObjects_fast();
   for(S32 i = 0; i < objects->size(); i++)
   {
      BfObject *bfobj = dynamic_cast<BfObject *>((*objects)[i]);
      if(bfobj->getClassRep() != NULL)  // Barriers and some other objects might not be ghostable...
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
         EXPECT_EQ(ghostingRecords[i].server, ghostingRecords[i].client) << " className=" << className;
      else
         EXPECT_NE(ghostingRecords[i].server, ghostingRecords[i].client) << " className=" << className;
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
   ClientGame *clientGame = gamePair.getClient(0);
   ServerGame *serverGame = gamePair.server;

   // Basic geometry to plug into polygon objects
   Vector<Point> geom;
   geom.push_back(Point(0,0));
   geom.push_back(Point(1,0));
   geom.push_back(Point(0,1));

   // TODO: Should not need this... we start an L somewhere in one of the tests and never shut it down
   if(!LuaScriptRunner::getL())
      ASSERT_TRUE(LuaScriptRunner::startLua((clientGame->getSettings()->getFolderManager()->getLuaDir())));
      
   lua_State *L = LuaScriptRunner::getL();

   ASSERT_EQ(1, serverGame->getTeamCount()) << "Need a team here or else the bfobj->lua_setTeam() below will fail!";

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
         bfobj->setExtent(Rect(0,0, 1,1));
         bfobj->GeomObject::setGeom(geom);

         // Lua testing
         lua_pushinteger(L, 1);
         bfobj->lua_setTeam(L);     // Assign bfobj to team 1 (first team, lua uses 1-index arrays)
         lua_pop(L, 1);

         lua_pushinteger(L, TEAM_HOSTILE);
         bfobj->lua_setTeam(L);     
         lua_pop(L, 1);

         luaPushPoint(L, 2.3f, 4.3f);
         bfobj->lua_setPos(L);
         lua_pop(L, 1);

         bfobj->addToGame(serverGame, serverGame->getLevel());
      }
      else
         delete bfobj;
   }

   LuaScriptRunner::shutdown();
}

   
}; // namespace Zap
