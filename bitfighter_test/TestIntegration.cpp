//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "game.h"
#include "stringUtils.h"
#include "ClientGame.h"
#include "ServerGame.h"
#include "GameManager.h"
#include "teleporter.h"
#include "PickupItem.h"
#include "barrier.h"
#include "gameType.h"

#include "TestUtils.h"
#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"
#include <string>

namespace Zap
{

using namespace std;

static const string joiner = " | ";

void checkTeleporter(Game *game, const string &geomString, S32 expectedDests)
{
   Vector<DatabaseObject *> fillVector;
   Teleporter *teleporter = NULL;

   game->getGameObjDatabase()->findObjects(TeleporterTypeNumber, fillVector);
   ASSERT_EQ(1, fillVector.size());
   teleporter = dynamic_cast<Teleporter *>(fillVector[0]);
   ASSERT_TRUE(teleporter);
   ASSERT_EQ(expectedDests, teleporter->getDestCount())  << "Wrong number of destinations";

   string actualGeomString = teleporter->getOrigin().toString();
   for(S32 i = 0; i < teleporter->getDestCount(); i++)
      actualGeomString +=  joiner + teleporter->getDest(i).toString();
   EXPECT_EQ(geomString, actualGeomString);
   EXPECT_FALSE(teleporter->isEngineered());

   // Test outline geometry on client only -- we don't care about this on the server, and don't really maintain it.
   // Basically want to verify that outline gets updated when teleporter geom changes.
   if(!game->isServer())
   {
      Point center = Rect(*teleporter->getOutline()).getCenter();
      EXPECT_NEAR(center.x, teleporter->getOrigin().x, 0.1);      // Close enough to pass?
      EXPECT_NEAR(center.y, teleporter->getOrigin().y, 0.1);
   }
}

void checkTeleporter(Teleporter *teleporter, Point *pts, S32 pointCount)
{
   TNLAssert(pointCount > 1, "This method only for use when testing 1 or more destinations!");

   Vector<Point> geom(pts, pointCount);

   string str = geom[0].toString();
   for(S32 i = 1; i < geom.size(); i++)
      str += joiner + geom[i].toString();

   teleporter->doSetGeom(geom);     // When a levelgen changes the geometry, this fn gets called
   GamePair::idle(10, 5);            // Idle for a while to allow propagation

   {
      SCOPED_TRACE("Testing SERVER game");
      checkTeleporter((Game *) GameManager::getServerGame(), str, pointCount - 1);
   }
   
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();
   for(S32 i = 0; i < clientGames->size(); i++)
   {
      SCOPED_TRACE("Testing CLIENT game " + itos(i));
      checkTeleporter((Game *) clientGames->get(i), str, pointCount - 1);
   }
}

// This test needs to be greatly expanded -- we should be testing all sorts of items here!
// Things to test: other objects, comments, long names, missing lines, duplicate lines, garbage lines, ids
TEST(IntegrationTest, LevelReadingAndItemPropagation)
{
   GamePair gamePair(getLevelCode1(), 3);

   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();
   ServerGame *serverGame = GameManager::getServerGame();

   // Idle for a while
   GamePair::idle(10, 5);

   Vector<DatabaseObject *> fillVector;

   // Test level item propagation
   // TestItem (placed @ 1,1)
   for(S32 i = 0; i < clientGames->size(); i++)
   {
      SCOPED_TRACE("i = " + itos(i));
      ClientGame *clientGame = clientGames->get(i);

      fillVector.clear();
      clientGame->getGameObjDatabase()->findObjects(TestItemTypeNumber, fillVector);
      ASSERT_EQ(1, fillVector.size()) << "Looks like object propagation is broken!";
      EXPECT_TRUE(fillVector[0]->getCentroid() == Point(255, 255));

      // RepairItem (placed @ 0,1, repop time = 10)
      fillVector.clear();
      clientGame->getGameObjDatabase()->findObjects(RepairItemTypeNumber, fillVector);
      ASSERT_EQ(1, fillVector.size());
      EXPECT_TRUE(fillVector[0]->getCentroid() == Point(0, 255));

      fillVector.clear();
      serverGame->getGameObjDatabase()->findObjects(RepairItemTypeNumber, fillVector);
      EXPECT_EQ(10, static_cast<RepairItem *>(fillVector[0])->getRepopDelay()); // <=== repopDelay is not sent to the client; on client will always be default

      // Wall (placed @ -1,-1 ==> -1,1  thickness = 40)
      fillVector.clear();
      clientGame->getGameObjDatabase()->findObjects(BarrierTypeNumber, fillVector);
      ASSERT_EQ(1, fillVector.size());
      Barrier *barrier = static_cast<Barrier *>(fillVector[0]);
      EXPECT_EQ("-255, -255 | -255, 255", barrier->mPoints[0].toString() + " | " + barrier->mPoints[1].toString());
      EXPECT_EQ(40, barrier->mWidth);

      // Teleporter (placed @ 5,5 ==> 10,10)
      {
         SCOPED_TRACE("ServerGame, original placement");
         checkTeleporter(serverGame, "1275, 1275 | 2550, 2550", 1);
      }
      {
         SCOPED_TRACE("ClientGame, original placement");
         checkTeleporter(clientGame, "1275, 1275 | 2550, 2550", 1);
      }
   }

   // Now move the teleporter on server, make sure changes propagate to client
   fillVector.clear();
   serverGame->getGameObjDatabase()->findObjects(TeleporterTypeNumber, fillVector);
   ASSERT_EQ(1, fillVector.size());
   Teleporter *teleporter = dynamic_cast<Teleporter *>(fillVector[0]);
   ASSERT_TRUE(teleporter);

   ///// Passing one point moves the origin -- shorter form below doesn't work when passing only one point
   {
      Point pts[] = { Point(30, 50) };   
      Vector<Point> geom(pts, ARRAYSIZE(pts));
      teleporter->doSetGeom(geom);        // When a levelgen changes the geometry, this fn gets called
      GamePair::idle(10, 5);              // Idle for a while to allow propagation
      {
         SCOPED_TRACE("1 ServerGame, after origin move");
         checkTeleporter(serverGame, "30, 50 | 2550, 2550", 1);
      }

      for(S32 i = 0; i < clientGames->size(); i++)
      {
         SCOPED_TRACE("1 ClientGame, after origin move #" + itos(i));
         checkTeleporter(clientGames->get(i), "30, 50 | 2550, 2550", 1);
      }
   }

   { 
      SCOPED_TRACE("2 Passing two points moves the origin and the dest");
      Point pts[] = { Point(100, 100), Point(150, 200) };    
      checkTeleporter(teleporter, pts, ARRAYSIZE(pts));
   }

   { 
      SCOPED_TRACE("3 Passing three points moves the origin and the dest and adds a second dest");
      Point pts[] = { Point(200, 220), Point(180, 300), Point(50, 60) };    
      checkTeleporter(teleporter, pts, ARRAYSIZE(pts));
   }

   ///// Passing one point moves the origin, leaving other dests intact
   {
      Point pts[] = { Point(80, 85) };   
      Vector<Point> geom(pts, ARRAYSIZE(pts));
      teleporter->doSetGeom(geom);        // When a levelgen changes the geometry, this fn gets called
      GamePair::idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("4 ServerGame, after origin move with 2 dests");
         checkTeleporter(serverGame, "80, 85 | 180, 300 | 50, 60", 2);
      }

      for(S32 i = 0; i < clientGames->size(); i++)
      {
         SCOPED_TRACE("4 ClientGame, after origin move with 2 dests #" + itos(i));
         checkTeleporter(clientGames->get(i), "80, 85 | 180, 300 | 50, 60", 2);
      }
   }

   { 
      SCOPED_TRACE("5 Passing two points moves the origin and the first dest, removing the second dest");
      Point pts[] = { Point(345, 555), Point(612, 123) };    
      checkTeleporter(teleporter, pts, ARRAYSIZE(pts));
   }

   // Add a dest with addDest()
   {
      teleporter->addDest(Point(19, 99)); // Party like it's
      GamePair::idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("addDest() test - ServerGame");
         checkTeleporter(serverGame, "345, 555 | 612, 123 | 19, 99", 2);
      }
      for(S32 i = 0; i < clientGames->size(); i++)
      {
         SCOPED_TRACE("addDest() test - ClientGame #" + itos(i));
         checkTeleporter(clientGames->get(i), "345, 555 | 612, 123 | 19, 99", 2);
      }
   }

   // Delete a dest with delDest()
   {
      teleporter->delDest(0);    // Delete first dest
      GamePair::idle(10, 5);      // Idle for a while to allow propagation

      {
         SCOPED_TRACE("delDest() test - ServerGame");
         checkTeleporter(serverGame, "345, 555 | 19, 99", 1);
      }                
      for(S32 i = 0; i < clientGames->size(); i++)
      {                                                   
         SCOPED_TRACE("delDest() test - ClientGame #" + itos(i));     
         checkTeleporter(clientGames->get(i), "345, 555 | 19, 99", 1);
      }
   }

   // Clear the dests with clearDests()
   {
      teleporter->clearDests(); 
      GamePair::idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("clearDests() test - ServerGame");
         checkTeleporter(serverGame, "345, 555", 0);
      }
      for(S32 i = 0; i < clientGames->size(); i++)
      {
         // Will be rendered in-game, will not teleport you anywhere... FYI
         SCOPED_TRACE("clearDests() test - ClientGame #" + itos(i));    
         checkTeleporter(clientGames->get(i), "345, 555", 0);
      }
   }

   /////
   // Test metadata propagation
   for(S32 i = 0; i < clientGames->size(); i++)
   {
      ClientGame *clientGame = clientGames->get(i);
      SCOPED_TRACE("metadata propagation i = " + itos(i)); 
      EXPECT_STREQ("Bluey", clientGame->getTeam(0)->getName().getString());                           // Team name
      EXPECT_STREQ("Test Level", clientGame->getGameType()->getLevelName()->getString());             // Quoted in level file
      EXPECT_STREQ("This is a basic test level", clientGame->getGameType()->getLevelDescription());   // Quoted in level file
      EXPECT_STREQ("level creator", clientGame->getGameType()->getLevelCredits()->getString());       // Not quoted in level file
   }
}   

};
