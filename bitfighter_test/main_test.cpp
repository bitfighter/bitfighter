// Bitfighter Tests

#define BF_TEST

#include "gtest/gtest.h"

#include "../zap/BfObject.h"
#include "../zap/EngineeredItem.h"
#include "../zap/gameType.h"
#include "../zap/LoadoutTracker.h"
#include "../zap/ServerGame.h"
#include "../zap/ship.h"
#include "../zap/HelpItemManager.h"
#include "../zap/ClientGame.h"
#include "../zap/ClientInfo.h"
#include "../zap/FontManager.h"
#include "../zap/config.h"
#include "../zap/SystemFunctions.h"
#include "../zap/UIManager.h"
#include "../zap/PickupItem.h"
#include "../zap/ChatCommands.h"
#include "../zap/teleporter.h"

#include "../zap/VideoSystem.h"

#include "../zap/UIGame.h"
#include "../zap/UIManager.h"
#include "../zap/UIEditorMenus.h"
#include "../zap/UIMenus.h"
#include "../zap/LoadoutIndicator.h"

#include "../zap/GeomUtils.h"
#include "../zap/stringUtils.h"

#include "TestUtils.h"
#include "LevelFilesForTesting.h"      // Contains sample levelcode for testing purposes
#include "EventKeyDefs.h"              // One big ugly macro for defining a bunch of vars related to key input events

#include "tnlNetObject.h"
#include "tnlGhostConnection.h"
#include "tnlPlatform.h"

#include <string>
#include <cmath>

#ifdef TNL_OS_WIN32
#  include <windows.h>   // For ARRAYSIZE
#endif

using namespace Zap;
using namespace std;

namespace Zap
{
void exitToOs(S32 errcode)                 { TNLAssert(false, "Should never be called!"); }
void shutdownBitfighter(ServerGame *game)  { TNLAssert(false, "Should never be called!"); };
}


class BfTest : public testing::Test
{
   // Config code can go here
};


TEST_F(BfTest, StringUtilsTests)
{
   EXPECT_TRUE(stringContainsAllTheSameCharacter("A"));
   EXPECT_TRUE(stringContainsAllTheSameCharacter("AA"));
   EXPECT_TRUE(stringContainsAllTheSameCharacter("AAA"));
   EXPECT_TRUE(stringContainsAllTheSameCharacter(""));

   EXPECT_FALSE(stringContainsAllTheSameCharacter("Aa"));
   EXPECT_FALSE(stringContainsAllTheSameCharacter("AB"));
} 


TEST_F(BfTest, IniSettingsPackUnpack)
{
   static const S32 count = 5;
   bool items[count];

   // Set all bits to false
   IniSettings::clearbits(items, count);
   ASSERT_FALSE(items[0] || items[1] || items[2] || items[3] || items[4]);

   // Set from proper length string
   IniSettings::iniStringToBitArray("YYNNN", items, count);
   ASSERT_TRUE(items[0] && items[1]);
   ASSERT_FALSE(items[2] || items[3] || items[4]);

   // Set all bits to false
   IniSettings::clearbits(items, count);
   ASSERT_FALSE(items[0] || items[1] || items[2] || items[3] || items[4]);

   // Set from short string
   IniSettings::iniStringToBitArray("YNY", items, count);
   ASSERT_TRUE(items[0] && items[2]);
   ASSERT_FALSE(items[1] || items[3] || items[4]);

   // Set from long string
   IniSettings::clearbits(items, count);
   IniSettings::iniStringToBitArray("NNYYYYYYYYYY", items, count);
   ASSERT_TRUE(items[2] && items[3] && items[4]);
   ASSERT_FALSE(items[0] || items[1]);

   // Set from bogus string
   IniSettings::clearbits(items, count);
   IniSettings::iniStringToBitArray("yyYXE", items, count);
   ASSERT_TRUE(items[2]);
   ASSERT_FALSE(items[0] || items[1] || items[3] || items[4]);

   // Now we know unpack works, so testing pack should be easy
   IniSettings::clearbits(items, count);
   string vals = "YYNNY";

   IniSettings::iniStringToBitArray(vals, items, count);
   ASSERT_EQ(IniSettings::bitArrayToIniString(items, count), vals);
}


TEST_F(BfTest, SettingsTests)
{
   Settings settings;
   settings.add(new Setting<string>     ("strName",  "defval",              "SettingName1", "Section", "description"));
   settings.add(new Setting<S32>        ("S32Name",  123,                   "SettingName2", "Section", "description"));
   settings.add(new Setting<YesNo>      ("YesNoYes", Yes,                   "YesNoYes",     "Section", "description"));
   settings.add(new Setting<YesNo>      ("YesNoNo",  No,                    "YesNoNo",      "Section", "description"));
   settings.add(new Setting<DisplayMode>("DispMode", DISPLAY_MODE_WINDOWED, "DispMode",     "Section", "description"));
   settings.add(new Setting<RelAbs>     ("RelAbs",   Relative,              "RelAbs",       "Section", "description"));

   // Check default values
   // Get a string representation of the value
   ASSERT_EQ("defval", settings.getStrVal("strName"));
   ASSERT_EQ("123",    settings.getStrVal("S32Name"));

   // Get the raw values
   ASSERT_EQ("defval", settings.getVal<string>("strName"));
   ASSERT_EQ(123,      settings.getVal<S32>("S32Name"));

   // Set some new values
   settings.setVal("strName", string("newVal"));
   settings.setVal("S32Name", 321);
   ASSERT_EQ("newVal", settings.getVal<string>("strName"));
   ASSERT_EQ(321,      settings.getVal<S32>("S32Name"));

   // Check YesNo/DisplayMode conversions

   // Check setting YesNo by string
   settings.getSetting("YesNoYes")->setValFromString("No");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));
   settings.getSetting("YesNoYes")->setValFromString("Yes");
   ASSERT_EQ(Yes, settings.getVal<YesNo>("YesNoYes"));

   // Different cases
   settings.getSetting("YesNoYes")->setValFromString("NO");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));
   settings.getSetting("YesNoYes")->setValFromString("yes");
   ASSERT_EQ(Yes, settings.getVal<YesNo>("YesNoYes"));

   // Unknwon values - should go to No
   settings.getSetting("YesNoYes")->setValFromString("abcdefg");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));
   // Empty string
   settings.getSetting("YesNoYes")->setValFromString("");
   ASSERT_EQ(No, settings.getVal<YesNo>("YesNoYes"));

   // Reset and check boolean equality
   settings.getSetting("YesNoYes")->setValFromString("Yes");
   settings.getSetting("YesNoNo")->setValFromString("No");
   ASSERT_EQ(true, settings.getVal<YesNo>("YesNoYes"));
   ASSERT_EQ(false, settings.getVal<YesNo>("YesNoNo"));

   // Relative/Absolute
   settings.getSetting("RelAbs")->setValFromString("absolute");
   ASSERT_EQ(Absolute, settings.getVal<RelAbs>("RelAbs"));
   settings.getSetting("RelAbs")->setValFromString("RELATIVE");
   ASSERT_EQ(Relative, settings.getVal<RelAbs>("RelAbs"));
}


static const string joiner = " | ";

static void checkTeleporter(Game *game, const string &geomString, S32 expectedDests)
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
      Point centroid = findCentroid(*teleporter->getOutline());
      EXPECT_FLOAT_EQ(centroid.x, teleporter->getOrigin().x);     // Use FLOAT comparisons to handle rounding errors
      EXPECT_FLOAT_EQ(centroid.y, teleporter->getOrigin().y);
   }
}


static void checkTeleporter(GamePair &gamePair, Teleporter *teleporter, Point *pts, S32 pointCount)
{
   TNLAssert(pointCount > 1, "This method only for use when testing 1 or more destinations!");

   ClientGame *clientGame = gamePair.client;
   ServerGame *serverGame = gamePair.server;

   Vector<Point> geom(pts, pointCount);

   string str = geom[0].toString();
   for(S32 i = 1; i < geom.size(); i++)
      str += joiner + geom[i].toString();

   teleporter->doSetGeom(geom);     // When a levelgen changes the geometry, this fn gets called
   gamePair.idle(10, 5);            // Idle for a while to allow propagation

   {
      SCOPED_TRACE("Testing SERVER game");
      checkTeleporter(serverGame, str, pointCount - 1);
   }
   {
      SCOPED_TRACE("Testing CLIENT game");
      checkTeleporter(clientGame, str, pointCount - 1);
   }
}


// This test needs to be greatly expanded -- we should be testing all sorts of items here!
// Things to test: other objects, comments, long names, missing lines, duplicate lines, garbage lines, ids
TEST_F(BfTest, LevelReadingAndItemPropagation)
{
   GamePair gamePair(getLevelCode1());
   ClientGame *clientGame = gamePair.client;
   ServerGame *serverGame = gamePair.server;

   // Idle for a while
   gamePair.idle(10, 5);

   Vector<DatabaseObject *> fillVector;

   // Test level item propagation
   // TestItem (placed @ 1,1)
   clientGame->getGameObjDatabase()->findObjects(TestItemTypeNumber, fillVector);
   EXPECT_EQ(1, fillVector.size()) << "Looks like object propagation is broken!";
   EXPECT_TRUE(fillVector[0]->getCentroid() == Point(255,255));

   // RepairItem (placed @ 0,1, repop time = 10)
   fillVector.clear();
   clientGame->getGameObjDatabase()->findObjects(RepairItemTypeNumber, fillVector);
   EXPECT_EQ(1, fillVector.size());
   EXPECT_TRUE(fillVector[0]->getCentroid() == Point(0,255));

   fillVector.clear();
   serverGame->getGameObjDatabase()->findObjects(RepairItemTypeNumber, fillVector);
   EXPECT_EQ(10, static_cast<RepairItem *>(fillVector[0])->getRepopDelay()); // <=== repopDelay is not sent to the client; on client will always be default

   // Wall (placed @ -1,-1 ==> -1,1  thickness = 40)
   fillVector.clear();
   clientGame->getGameObjDatabase()->findObjects(BarrierTypeNumber, fillVector);
   EXPECT_EQ(1, fillVector.size());
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
      gamePair.idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("1 ServerGame, after origin move");
         checkTeleporter(serverGame, "30, 50 | 2550, 2550", 1);
      }
      {
         SCOPED_TRACE("1 ClientGame, after origin move");
         checkTeleporter(clientGame, "30, 50 | 2550, 2550", 1);
      }
   }

   { 
      SCOPED_TRACE("2 Passing two points moves the origin and the dest");
      Point pts[] = { Point(100, 100), Point(150, 200) };    
      checkTeleporter(gamePair, teleporter, pts, ARRAYSIZE(pts));
   }

   { 
      SCOPED_TRACE("3 Passing three points moves the origin and the dest and adds a second dest");
      Point pts[] = { Point(200, 220), Point(180, 300), Point(50, 60) };    
      checkTeleporter(gamePair, teleporter, pts, ARRAYSIZE(pts));
   }

   ///// Passing one point moves the origin, leaving other dests intact
   {
      Point pts[] = { Point(80, 85) };   
      Vector<Point> geom(pts, ARRAYSIZE(pts));
      teleporter->doSetGeom(geom);        // When a levelgen changes the geometry, this fn gets called
      gamePair.idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("4 ServerGame, after origin move with 2 dests");
         checkTeleporter(serverGame, "80, 85 | 180, 300 | 50, 60", 2);
      }
      {
         SCOPED_TRACE("4 ClientGame, after origin move with 2 dests");
         checkTeleporter(clientGame, "80, 85 | 180, 300 | 50, 60", 2);
      }
   }

   { 
      SCOPED_TRACE("5 Passing two points moves the origin and the first dest, removing the second dest");
      Point pts[] = { Point(345, 555), Point(612, 123) };    
      checkTeleporter(gamePair, teleporter, pts, ARRAYSIZE(pts));
   }

   // Add a dest with addDest()
   {
      teleporter->addDest(Point(19, 99)); // Party like it's
      gamePair.idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("addDest() test - ServerGame");
         checkTeleporter(serverGame, "345, 555 | 612, 123 | 19, 99", 2);
      }
      {
         SCOPED_TRACE("addDest() test - ClientGame");
         checkTeleporter(clientGame, "345, 555 | 612, 123 | 19, 99", 2);
      }
   }

   // Delete a dest with delDest()
   {
      teleporter->delDest(0);    // Delete first dest
      gamePair.idle(10, 5);      // Idle for a while to allow propagation

      {
         SCOPED_TRACE("delDest() test - ServerGame");
         checkTeleporter(serverGame, "345, 555 | 19, 99", 1);
      }                                                   
      {                                                   
         SCOPED_TRACE("delDest() test - ClientGame");     
         checkTeleporter(clientGame, "345, 555 | 19, 99", 1);
      }
   }

   // Clear the dests with clearDests()
   {
      teleporter->clearDests(); 
      gamePair.idle(10, 5);               // Idle for a while to allow propagation

      {
         SCOPED_TRACE("clearDests() test - ServerGame");
         checkTeleporter(serverGame, "345, 555", 0);
      }
      {
         SCOPED_TRACE("clearDests() test - ClientGame");    // Will be rendered in-game, will not teleport you anywhere... FYI
         checkTeleporter(clientGame, "345, 555", 0);
      }
   }

   /////
   // Test metadata propagation
   EXPECT_STREQ("Bluey", clientGame->getTeam(0)->getName().getString());                                       // Team name
   EXPECT_STREQ("Test Level",                 clientGame->getGameType()->getLevelName()->getString());         // Quoted in level file
   EXPECT_STREQ("This is a basic test level", clientGame->getGameType()->getLevelDescription()->getString());  // Quoted in level file
   EXPECT_STREQ("level creator",              clientGame->getGameType()->getLevelCredits()->getString());      // Not quoted in level file
}


TEST_F(BfTest, LevelMenuSelectUserInterfaceTests) 
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(settings.get(), false);   
   ClientGame game(addr, settings, new UIManager());    // ClientGame destructor will clean up UIManager

   // Want to test getIndexOfNext(), which is a slightly complex function.  Need to start by setting up a menu.
   LevelMenuSelectUserInterface *ui = new LevelMenuSelectUserInterface(&game);      // Cleaned up when game goes out of scope

   // These should be alphabetically sorted
   ui->addMenuItem(new MenuItem("Aardvark"));    //  0
   ui->addMenuItem(new MenuItem("Assinine"));    //  1
   ui->addMenuItem(new MenuItem("Bouy"));        //  2
   ui->addMenuItem(new MenuItem("Boy"));         //  3
   ui->addMenuItem(new MenuItem("C"));           //  4
   ui->addMenuItem(new MenuItem("Cat"));         //  5
   ui->addMenuItem(new MenuItem("Cc"));          //  6
   ui->addMenuItem(new MenuItem("Chop"));        //  7
   ui->addMenuItem(new MenuItem("Chump"));       //  8
   ui->addMenuItem(new MenuItem("Dog"));         //  9
   ui->addMenuItem(new MenuItem("Doug"));        // 10
   ui->addMenuItem(new MenuItem("Eat"));         // 11
   ui->addMenuItem(new MenuItem("Eating"));      // 12
   ui->addMenuItem(new MenuItem("Eel"));         // 13
   ui->addMenuItem(new MenuItem("Eels"));        // 14
   ui->addMenuItem(new MenuItem("Eggs"));        // 15


   // Some random checks
   ui->selectedIndex = 1;
   ASSERT_EQ(ui->getIndexOfNext("a"), 0);
   ASSERT_EQ(ui->getIndexOfNext("boy"), 3);
   ASSERT_EQ(ui->getIndexOfNext("c"), 4);
   ASSERT_EQ(ui->getIndexOfNext("ch"), 7);
   ASSERT_EQ(ui->getIndexOfNext("cho"), 7);
   ASSERT_EQ(ui->getIndexOfNext("chop"), 7);

   // Check cycling of the Cs
   ui->selectedIndex = 3;
   ASSERT_EQ(ui->getIndexOfNext("c"), 4);
   ui->selectedIndex = 4;
   ASSERT_EQ(ui->getIndexOfNext("c"), 5);
   ui->selectedIndex = 5;
   ASSERT_EQ(ui->getIndexOfNext("c"), 6);
   ui->selectedIndex = 6;
   ASSERT_EQ(ui->getIndexOfNext("c"), 7);
   ui->selectedIndex = 7;
   ASSERT_EQ(ui->getIndexOfNext("c"), 8);
   ui->selectedIndex = 8;
   ASSERT_EQ(ui->getIndexOfNext("c"), 4);

   // Check wrapping
   ui->selectedIndex = 9;
   ASSERT_EQ(ui->getIndexOfNext("a"), 0);
   ui->selectedIndex = 15;     // last item
   ASSERT_EQ(ui->getIndexOfNext("a"), 0);

   // Check repeated hammering on current item
   ui->selectedIndex = 12;
   ASSERT_EQ(ui->getIndexOfNext("e"), 13);    // Single letter advances to next of that letter
   ASSERT_EQ(ui->getIndexOfNext("ea"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eat"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eati"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eatin"), 12);
   ASSERT_EQ(ui->getIndexOfNext("eating"), 12);

   // Check for not found items -- should return current index
   ASSERT_EQ(ui->getIndexOfNext("eatingx"), 12); 
   ASSERT_EQ(ui->getIndexOfNext("flummoxed"), 12); 

   ui->selectedIndex = 8;
   ASSERT_EQ(ui->getIndexOfNext("chop"), 7); 
}


TEST_F(BfTest, LoadoutTrackerTests) 
{
   LoadoutTracker t1;
   ASSERT_FALSE(t1.isValid());      // New trackers are not valid
   ASSERT_EQ(t1.toString(), "");   

   // Assign with a vector of items
   U8 items[] = { ModuleSensor, ModuleArmor, WeaponBounce, WeaponPhaser, WeaponBurst };
   Vector<U8> vals(items, ARRAYSIZE(items));

   t1.setLoadout(Vector<U8>(items, ARRAYSIZE(items)));
   ASSERT_EQ(t1.toString(), "Sensor,Armor,Bouncer,Phaser,Burst");

   // Check equality and assignment 
   LoadoutTracker t2;
   ASSERT_NE(t1, t2);
   t2 = t1;
   ASSERT_EQ(t1, t2);
   ASSERT_EQ(t2.toString(), "Sensor,Armor,Bouncer,Phaser,Burst");

   // Check creating from a string
   LoadoutTracker t3("Sensor,Armor,Bouncer,Phaser,Burst");
   ASSERT_EQ(t2, t3);

   // Check dumping to Vector
   Vector<U8> outItems = t3.toU8Vector();
   ASSERT_EQ(outItems.size(), ShipModuleCount + ShipWeaponCount);
   for(S32 i = 0; i < outItems.size(); i++)
      ASSERT_EQ(outItems[i], items[i]);
}


static void packUnpack(Move move1, Move &move2)
{
   PacketStream stream;    // Create a stream

   move1.pack(&stream, NULL, false);   // Write the move
   stream.setBitPosition(0);           // Move the stream's pointer back to the beginning
   move2.unpack(&stream, false);       // Read the move
}


// Generic pack/unpack tester -- can feed it any class that supports pack/unpack
template <class T>
void packUnpack(T input, T &output, U32 mask = 0xFFFFFFFF)
{
   BitStream stream;       
   GhostConnection conn;
   
   output.markAsGhost(); 

   input.packUpdate(&conn, mask, &stream);   // Write the object
   stream.setBitPosition(0);                 // Move the stream's pointer back to the beginning
   output.unpackUpdate(&conn, &stream);      // Read the object back
}


TEST_F(BfTest, LoadoutManagementTests)
{
   ServerGame *serverGame = newServerGame();
   GameType *gt = new GameType();    // Cleaned up by database
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());

   Ship *s = new Ship();             // Cleaned up by database
   s->addToGame(serverGame, serverGame->getGameObjDatabase());

   // Tests to ensure that currently selected weapon stays the same when changing loadout
   s->setLoadout(LoadoutTracker("Shield,Repair,Burst,Phaser,Bouncer"));        // Set initial loadout
   s->selectWeapon(2);                                                         // Make bouncers active weapon
   s->setLoadout(LoadoutTracker("Armor,Sensor,Phaser,Bouncer,Seeker"), false); // Set loadout in noisy mode
   ASSERT_EQ(s->getActiveWeapon(), WeaponBounce);                              // Bouncer should still be active weapon
   s->setLoadout(LoadoutTracker("Armor,Shield,Triple,Mine,Bouncer"), true);    // Set loadout in silent mode
   ASSERT_EQ(s->getActiveWeapon(), WeaponBounce);                              // Bouncer should _still_ be active weapon
   s->setLoadout(LoadoutTracker("Armor,Shield,Triple,Phaser,Mine"), false);    // Set loadout in noisy mode
   ASSERT_EQ(s->getActiveWeapon(), WeaponTriple);                              // Bouncer not in loadout, should select first weap (Triple)
   s->selectWeapon(2);                                                         // Select 3rd weapon, Mine
   ASSERT_EQ(s->getActiveWeapon(), WeaponMine);                                // Confirm we've selected it
   s->setLoadout(LoadoutTracker("Armor,Shield,Seeker,Phaser,Triple"), true);   // Set loadout in silent mode
   ASSERT_EQ(s->getActiveWeapon(), WeaponSeeker);                              // Mine not in loadout, should select first weap (Seeker)

   // Tests to ensure that resource items get dropped when changing loadout away from engineer.  We'll also add a flag
   // and verify that the flag is not similarly dropped.  These cleaned up by database.
   ResourceItem *r = new ResourceItem();
   FlagItem     *f = new FlagItem();

   r->addToGame(serverGame, serverGame->getGameObjDatabase());
   f->addToGame(serverGame, serverGame->getGameObjDatabase());

   s->setLoadout(LoadoutTracker("Engineer,Shield,Triple,Mine,Bouncer"));       // Ship has engineer
   r->mountToShip(s);
   f->mountToShip(s);
   ASSERT_TRUE(s->isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s->isCarryingItem(FlagTypeNumber));
   s->setLoadout(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"), false);   // Ship does not have engineer
   ASSERT_FALSE(s->isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s->isCarryingItem(FlagTypeNumber));

   // Same test, in silent mode
   s->setLoadout(LoadoutTracker("Engineer,Shield,Triple,Mine,Bouncer"));       // Ship has engineer
   r->mountToShip(s);
   ASSERT_TRUE(s->isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s->isCarryingItem(FlagTypeNumber));
   s->setLoadout(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"), true);    // Ship does not have engineer
   ASSERT_FALSE(s->isCarryingItem(ResourceItemTypeNumber));
   ASSERT_TRUE(s->isCarryingItem(FlagTypeNumber));

   delete serverGame;
}


TEST_F(BfTest, LoadoutIndicatorTests)
{
   Address addr;
   GameSettingsPtr settings = GameSettingsPtr(new GameSettings());

   // Need to initialize FontManager to use ClientGame... use false to avoid hassle of locating font files.
   // False will tell the FontManager to only use internally defined fonts; any TTF fonts will be replaced with Roman.
   FontManager::initialize(settings.get(), false);   
   ClientGame game(addr, settings, new UIManager());    // ClientGame destructor will clean up UIManager

   UI::LoadoutIndicator indicator;
   indicator.newLoadoutHasArrived(LoadoutTracker("Turbo,Shield,Triple,Mine,Bouncer"));     // Sets the loadout

   // Make sure the calculated width matches the rendered width
   ASSERT_EQ(indicator.render(&game), indicator.getWidth());
}


TEST_F(BfTest, ShipTests)
{
   Ship serverShip, clientShip;
   ASSERT_TRUE(serverShip.isServerCopyOf(clientShip));   // New ships start out equal

   // Set some shippy stuff on serverShip
   serverShip.setRenderPos(Point(100,100));
   serverShip.setLoadout(LoadoutTracker("Shield,Repair,Burst,Phaser,Bouncer"));

   ASSERT_FALSE(serverShip.isServerCopyOf(clientShip));  // After updating server copy, ships no longer equal

   packUnpack(serverShip, clientShip);                   // Transmit server details to client

   ASSERT_TRUE(serverShip.isServerCopyOf(clientShip));   // Ships should be equal again
}


TEST_F(BfTest, EngineerTests)
{
   GamePair gamePair(getLevelCodeForTestingEngineer1()); // See def for description of level
   ClientGame *clientGame       = gamePair.client;
   ServerGame *serverGame       = gamePair.server;
   GameSettings *clientSettings = clientGame->getSettings();
   GameUserInterface *gameUI    = clientGame->getUIManager()->getUI<GameUserInterface>();

   DEFINE_KEYS_AND_EVENTS(clientSettings);

   // Idle for a while, let things settle
   gamePair.idle(10, 5);

   // Verify that engineer is enabled
   ASSERT_TRUE(serverGame->getGameType()->isEngineerEnabled());
   ASSERT_TRUE(clientGame->getGameType()->isEngineerEnabled());

   ASSERT_TRUE(serverGame->getClientInfo(0)->getShip()->isInZone(LoadoutZoneTypeNumber)); // Check level is as we expected

   // Add engineer to current loadout
   ASSERT_FALSE(gameUI->isHelperActive(HelperMenu::LoadoutHelperType));
   gameUI->activateHelper(HelperMenu::LoadoutHelperType);
   ASSERT_TRUE(gameUI->isHelperActive(HelperMenu::LoadoutHelperType));

   gameUI->onKeyDown(LOADOUT_KEY_ENGR);      gameUI->onKeyDown(LOADOUT_KEY_REPAIR);                                           // First 2 modules...
   gameUI->onKeyDown(LOADOUT_KEY_PHASER);    gameUI->onKeyDown(LOADOUT_KEY_BOUNCE);    gameUI->onKeyDown(LOADOUT_KEY_TRIPLE); // ...then 3 weapons

   // On this level, the ship spawn is inside a loadout zone, so loadout should take effect immediately
   gamePair.idle(10, 5);
   ASSERT_EQ("Engineer,Repair,Phaser,Bouncer,Triple", serverGame->getClientInfo(0)->getShip()->getLoadoutString());
   ASSERT_EQ("Engineer,Repair,Phaser,Bouncer,Triple", clientGame->getLocalPlayerShip()->getLoadoutString());

   // Fly down to pick up resource item -- to get flying to work, need to create events at a very basic level
   Point startPos = clientGame->getLocalPlayerShip()->getActualPos();
   Event::onEvent(clientGame, &EventDownPressed);
   gamePair.idle(100, 5);
   Event::onEvent(clientGame, &EventDownReleased);
   Point endPos = clientGame->getLocalPlayerShip()->getActualPos();
   ASSERT_TRUE(startPos.distSquared(endPos) > 0) << "Ship did not move!!";
   ASSERT_TRUE(clientGame->getLocalPlayerShip()->isCarryingItem(ResourceItemTypeNumber));

   // Time to engineer!
   gameUI->onKeyDown(KEY_MOD1);
   ASSERT_TRUE(gameUI->isHelperActive(HelperMenu::EngineerHelperType)) << "Expect engineer helper menu to be active!";

   InputCode key = EngineerHelper::getInputCodeForOption(EngineeredTeleporterEntrance, true);
   gameUI->onKeyDown(key);
   ASSERT_FALSE(static_cast<const EngineerHelper *>(gameUI->getActiveHelper())->isMenuBeingDisplayed());
   gameUI->onKeyDown(KEY_MOD1);     // Place entrance
   gameUI->onKeyDown(KEY_MOD1);     // Place exit
   gamePair.idle(100, 5);           // Let things mellow
   Vector<DatabaseObject *> fillVector;
   clientGame->getGameObjDatabase()->findObjects(TeleporterTypeNumber, fillVector);
   EXPECT_EQ(1, fillVector.size()) << "Expected a teleporter!";
}


TEST_F(BfTest, MoveTests)
{
   Move move1, move2;
   
   // Demonstrate testing of a basic pack/unpack cycle... but prepare basically already does this so I'm not 
   // sure this test is really that interesting
   move1.set(0.125f, 0.75f, 33.3f);    
   move1.prepare();
   packUnpack(move1, move2);
   ASSERT_TRUE(move1.isEqualMove(&move2)); 

   // An area of concern by an earlier dev is that angles might get transmitted incorreclty.  I agree that the math
   // is confusing, so let's create some tests to verify that prepare() normalizes the input angle to between
   // 0 and 2pi, which will then be handled properly by writeInt (and nicely keeps angles sane).

   // Obvious cases
   move1.angle = FloatHalfPi;             move1.prepare();  ASSERT_EQ(move1.angle, FloatHalfPi);                     
   move1.angle = FloatPi + FloatHalfPi;   move1.prepare();  ASSERT_EQ(move1.angle, FloatPi + FloatHalfPi);         

   // Negative angles
   move1.angle = -FloatHalfPi;            move1.prepare();  ASSERT_EQ(move1.angle, FloatPi + FloatHalfPi);  // -1/4 turn = 3/4 turn  
   move1.angle = -FloatPi - FloatHalfPi;  move1.prepare();  ASSERT_EQ(move1.angle, FloatHalfPi);            // -3/4 turn = 1/4 turn  

   // Exactly one turn
   move1.angle =  Float2Pi;               move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Wrap exactly once, pos. dir
   move1.angle = -Float2Pi;               move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Wrap exactly once, neg. dir

   // Large angles
   move1.angle =  Float2Pi + FloatHalfPi;  move1.prepare();  ASSERT_EQ(move1.angle, FloatHalfPi);            // Wrap in pos. dir         
   move1.angle = -Float2Pi - FloatHalfPi; move1.prepare();   ASSERT_EQ(move1.angle, FloatPi + FloatHalfPi);  // Wrap in neg. dir 

   // Really large angles -- we'll never see these in the game
   move1.angle = 432 * Float2Pi;          move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Big angles 
   move1.angle =  -9 * Float2Pi;          move1.prepare();  ASSERT_EQ(move1.angle, 0);  // Big neg. angles 
} 


TEST_F(BfTest, GameTypeTests)
{
   GameType gt;

   // Check the maxPlayersPerTeam fn
   //                                players--v  v--teams
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam( 1, 1), 1);
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam( 1, 2), 1);
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam(10, 5), 2);
   ASSERT_EQ(gt.getMaxPlayersPerBalancedTeam(11, 5), 3);
}


static void checkQueues(const UI::HelpItemManager &himgr, S32 highSize, S32 lowSize, S32 displaySize, HelpItem displayItem = UnknownHelpItem)
{
   // Check that queue sizes match what we specified
   ASSERT_EQ(himgr.getHighPriorityQueue()->size(), highSize);
   ASSERT_EQ(himgr.getLowPriorityQueue()->size(), lowSize);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), displaySize);

   // Check if displayItem is being displayed, unless displayItem is UnknownHelpItem
   if(displayItem != UnknownHelpItem && himgr.getHelpItemDisplayList()->size() > 0)
      ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), displayItem);
}


// Full cycle is PacedTimerPeriod; that is, every PacedTimerPeriod ms, a new item is displayed
// Within that cycle, and item is displayed for HelpItemDisplayPeriod, then faded for getRollupPeriod,
// at which point it is expired and removed from the screen.  Then we must wait until the remainder of 
// PacedTimerPeriod has elapsed before a new item will be shown.  The following methods try to makes sense of that.
static S32 idleUntilItemExpiredMinusOne(UI::HelpItemManager &himgr, const ClientGame *game)
{
   S32 lastRollupPeriod = himgr.getRollupPeriod(0);
   himgr.idle(himgr.HelpItemDisplayPeriod,  game);   // Can't combine these periods... needs to be two different idle cycles
   himgr.idle(lastRollupPeriod - 1, game);   // First cycle to enter fade mode, second to exhaust fade timer
   return lastRollupPeriod;
}


// Idles HelpItemDisplayPeriod + getRollupPeriod(), long enough for a freshly displayed item to disappear
static S32 idleUntilItemExpired(UI::HelpItemManager &himgr, const ClientGame *game)
{
   S32 lastRollupPeriod = idleUntilItemExpiredMinusOne(himgr, game);
   himgr.idle(1, game);
   return lastRollupPeriod;
}

static void idleRemainderOfFullCycleMinusOne(UI::HelpItemManager &himgr, const ClientGame *game, S32 lastRollupPeriod)
{
   himgr.idle(himgr.PacedTimerPeriod - himgr.HelpItemDisplayPeriod - lastRollupPeriod - 1, game);
}

// Assumes we've idled HelpItemDisplayPeriod + getRollupPeriod(), idles remainder of PacedTimerPeriod
static void idleRemainderOfFullCycle(UI::HelpItemManager &himgr, const ClientGame *game, S32 lastRollupPeriod)
{
   idleRemainderOfFullCycleMinusOne(himgr, game, lastRollupPeriod);
   himgr.idle(1, game);
}

static void idleFullCycleMinusOne(UI::HelpItemManager &himgr, const ClientGame *game)
{
   S32 lastRollupPeriod = idleUntilItemExpired(himgr, game);
   idleRemainderOfFullCycleMinusOne(himgr, game, lastRollupPeriod);
}

// Idles full PacedTimerPeriod, broken into strategic parts
static void idleFullCycle(UI::HelpItemManager &himgr, const ClientGame *game)
{
   idleFullCycleMinusOne(himgr, game);
   himgr.idle(1, game);
}


TEST_F(BfTest, HelpItemManagerTests)
{
   ClientGame *game = newClientGame();

   // Need to add a gameType because gameType is where the game timer is managed
   GameType *gameType = new GameType();    // Will be deleted in game destructor
   gameType->addToGame(game, game->getGameObjDatabase());

   HelpItem helpItem = NexusSpottedItem;

   // Since different items take different times to rollup when their display period is over, and since we can only
   // determine that time period when items are being displayed, we will grab that value when we can and store it for
   // future use.  We'll always store it in lastRollupPeriod.
   S32 lastRollupPeriod;

   /////
   // Here we verify that displayed help items are corretly stored in the INI

   UI::HelpItemManager himgr(game->getSettings());
   ASSERT_EQ(himgr.getAlreadySeenString()[helpItem], 'N');  // Item not marked as seen originally

   himgr.addInlineHelpItem(helpItem);                       // Add up a help item -- should not get added during initial delay period
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify no help item is being displayed

   himgr.idle(himgr.InitialDelayPeriod, game);              // Idle out of initial delay period

   himgr.addInlineHelpItem(helpItem);                       // Requeue helpItem -- initial delay period has expired, so should get added
   ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), helpItem);

   ASSERT_EQ(himgr.getAlreadySeenString()[helpItem], 'Y');  // Item marked as seen
   ASSERT_EQ(game->getSettings()->getIniSettings()->mSettings.getVal<string>("HelpItemsAlreadySeenList"), himgr.getAlreadySeenString());  // Verify changes made it to the INI

   idleUntilItemExpired(himgr, game);

   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify help item is no longer displayed
   himgr.addInlineHelpItem(TeleporterSpotedItem);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Still in flood control period, new item not added

   himgr.idle(himgr.FloodControlPeriod, game);
   himgr.addInlineHelpItem(helpItem);                       // We've already added this item, so it should not be displayed again
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify help item is not displayed again
   himgr.addInlineHelpItem(TeleporterSpotedItem);           // Now, new item should be added (we tried adding this above, and it didn't go)
   ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), TeleporterSpotedItem);   // Verify help item is now visible

   /////
   // Test that high priority queued items are displayed before lower priority items

   himgr = UI::HelpItemManager(game->getSettings());        // Get a new, clean manager
   
   HelpItem highPriorityItem = ControlsKBItem;
   HelpItem lowPriorityItem  = CmdrsMapItem;
   HelpItem gameStartPriorityItem = NexGameStartItem;

   ASSERT_EQ(himgr.getItemPriority(highPriorityItem),      UI::HelpItemManager::PacedHigh);    // Prove that these items
   ASSERT_EQ(himgr.getItemPriority(lowPriorityItem),       UI::HelpItemManager::PacedLow);     // have the expected priority
   ASSERT_EQ(himgr.getItemPriority(gameStartPriorityItem), UI::HelpItemManager::GameStart);     

   himgr.addInlineHelpItem(lowPriorityItem);    // Queue both items up
   himgr.addInlineHelpItem(highPriorityItem);

   // Verify that both have been queued -- one item in each queue, and there are no items in the display list
   checkQueues(himgr, 1, 1, 0);

   // Now queue the gameStartPriorityItem... it should not get queued, and things should remain as they were
   himgr.addInlineHelpItem(gameStartPriorityItem);
   checkQueues(himgr, 1, 1, 0);

   himgr.idle(himgr.InitialDelayPeriod - 1, game);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);

   himgr.idle(1, game);    // InitialDelayPeriod has expired

   // Verify that our high priority item has been moved to the active display list
   checkQueues(himgr, 0, 1, 1, highPriorityItem);

   S32 rollupPeriod;
   rollupPeriod = himgr.getRollupPeriod(0);
   himgr.idle(himgr.HelpItemDisplayPeriod, game); himgr.idle(rollupPeriod - 1, game);  // (can't combine)

   // Verify nothing has changed
   checkQueues(himgr, 0, 1, 1, highPriorityItem);
   
   himgr.idle(1, game);    // Now, help item should no longer be displayed
   checkQueues(himgr, 0, 1, 0);

   himgr.idle(himgr.PacedTimerPeriod - himgr.HelpItemDisplayPeriod - rollupPeriod - 1, game); // Idle to cusp of pacedTimer expiring
   // Verify nothing has changed
   checkQueues(himgr, 0, 1, 0);
   himgr.idle(1, game);    // pacedTimer has expired, a new pacedItem will be added to the display list

   // Second message should now be moved from the lowPriorityQueue to the active display list, and will be visible
   lastRollupPeriod = himgr.getRollupPeriod(0);
   checkQueues(himgr, 0, 0, 1, lowPriorityItem);

   // Idle until list is clear
   idleUntilItemExpired(himgr, game);
   checkQueues(himgr, 0, 0, 0);       // No items displayed
   idleRemainderOfFullCycle(himgr, game, lastRollupPeriod);

   // Try adding our gameStartPriorityItem now... since there is nothing in the high priority queue, it should add fine
   himgr.addInlineHelpItem(gameStartPriorityItem);
   checkQueues(himgr, 1, 0, 0);
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(himgr, 0, 0, 1);       // No items displayed
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 0, 0);       // No items displayed

   // Make sure an item in the low priority queue won't impede the addition of a gameStartPriorityItem
   himgr.clearAlreadySeenList();    // Allows us to add these same items again
   himgr.addInlineHelpItem(lowPriorityItem);
   checkQueues(himgr, 0, 1, 0);
   himgr.addInlineHelpItem(gameStartPriorityItem);
   checkQueues(himgr, 1, 1, 0);
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(himgr, 0, 1, 1, gameStartPriorityItem);       // gameStartPriority item displayed
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 0, 1);
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 0, 0);

   // Verify bug with two high priority paced items preventing the first from being displayed
   // Start fresh with a new HelpItemManager
   himgr = UI::HelpItemManager(game->getSettings());
   // Verify they have HighPaced priority, then queue them up
   ASSERT_EQ(himgr.getItemPriority(ControlsKBItem),      UI::HelpItemManager::PacedHigh);
   ASSERT_EQ(himgr.getItemPriority(ControlsModulesItem), UI::HelpItemManager::PacedHigh);
   himgr.addInlineHelpItem(ControlsKBItem);
   himgr.addInlineHelpItem(ControlsModulesItem);
   checkQueues(himgr, 2, 0, 0);                       // Two high priority items queued, none displayed

   himgr.idle(himgr.InitialDelayPeriod, game);        // Wait past the intial delay period; first item will be displayed
   checkQueues(himgr, 1, 0, 1, ControlsKBItem);       // One item queued, one displayed

   idleUntilItemExpiredMinusOne(himgr, game);
   checkQueues(himgr, 1, 0, 1, ControlsKBItem);       // One item queued, one displayed

   lastRollupPeriod = himgr.getRollupPeriod(0);
   himgr.idle(1, game);
   idleRemainderOfFullCycleMinusOne(himgr, game, lastRollupPeriod);
   checkQueues(himgr, 1, 0, 0);
   himgr.idle(1, game);
   checkQueues(himgr, 0, 0, 1, ControlsModulesItem);  // No items queued, one displayed

   lastRollupPeriod = himgr.getRollupPeriod(0);

   idleUntilItemExpired(himgr, game);                 // Clear the decks
   checkQueues(himgr, 0, 0, 0);
   checkQueues(himgr, 0, 0, 0);

   himgr.clearAlreadySeenList();    // Allows us to add the same items again

   // Check the increasingly complex logic inside moveItemFromQueueToActiveList()
   // We already know that the logic with high-priority and low-priority lists is working under ordinary circumstances
   // So let's ensure it is working for the special situation of AddBotsItem, which should be ignored if there are bots in the game 
   // Remember that we are in mid-cycle, so we need to finish the cycle* before new items are displayed
   himgr.addInlineHelpItem(AddBotsItem);
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(himgr, 0, 2, 0);  // Both items should be queued, but none displayed

   idleRemainderOfFullCycle(himgr, game, lastRollupPeriod); // Cycle is completed, ready for a new item
   checkQueues(himgr, 0, 1, 1, AddBotsItem);                // No bots are in game ==> AddBotsItem should be displayed

   // Reset
   idleFullCycle(himgr, game);
   checkQueues(himgr, 0, 0, 1, CmdrsMapItem);               // One item displayed, none in the queues
   idleFullCycleMinusOne(himgr, game);
   checkQueues(himgr, 0, 0, 0);                             // No items dispalyed, queues are empty

   himgr.clearAlreadySeenList();    // Allows us to add the same items again

   // Add the same two items again
   himgr.addInlineHelpItem(AddBotsItem);
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(himgr, 0, 2, 0);     // Both items should be queued, none yet displayed

   // Create a ClientInfo for a robot -- it will be deleted by ClientGame destructor
   RemoteClientInfo *clientInfo = new RemoteClientInfo(game, "Robot", true, 0, 0, 0, true, ClientInfo::RoleNone, false, false);
   clientInfo->setTeamIndex(0);        // Assign it to our team
   game->addToClientList(clientInfo);  // Add it to the game
   ASSERT_EQ(game->getBotCount(), 1);  // Confirm there is a bot in the game

   himgr.idle(1, game);
   checkQueues(himgr, 0, 1, 1, CmdrsMapItem); // With bot in game, AddBotsItem should be bypassed, should see CmdrsMapItem

   idleUntilItemExpired(himgr, game);
   checkQueues(himgr, 0, 1, 0);              // Bot help still not showing
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(himgr, 0, 1, 0);              // Bot help still not showing
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(himgr, 0, 1, 0);              // And again!

   game->removeFromClientList(clientInfo);   // Remove the bot
   ASSERT_EQ(game->getBotCount(), 0);        // Confirm no bots in the game

   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);      // Not exact, just a big chunk of time
   checkQueues(himgr, 0, 0, 1, AddBotsItem); // With no bot, AddBotsItem will be displayed, though not necessarily immediately

   delete game;
}


TEST_F(BfTest, ClientGameTests) 
{
   ClientGame *game = newClientGame();

   // TODO: Add some tests

   delete game;
}


// Tests related to kill streaks
TEST_F(BfTest, KillStreakTests)
{
   ServerGame *game = newServerGame();
   GameType *gt = new GameType();      // Cleaned up in game destructor
   gt->addToGame(game, game->getGameObjDatabase());

   GameConnection conn;
   conn.setObjectMovedThisGame(true);     // Hacky way to avoid getting disconnected when the game is over, which will 
                                          // cause the tests to crash.  Will probably find a better way as we develop further.
   FullClientInfo *ci = new FullClientInfo(game, &conn, "Noman", false);      // Cleaned up somewhere
   conn.setClientInfo(ci);

   LevelInfo levelInfo("Level", BitmatchGame);     // Need a levelInfo for when we change levels

   game->addClient(ci);
   game->addLevel(levelInfo);
   
   game->setGameTime(1.0f / 60.0f); // 1 second, in minutes

   ASSERT_EQ(0, game->getClientInfo(0)->getKillStreak());
   game->getClientInfo(0)->addKill();
   ASSERT_EQ(1, game->getClientInfo(0)->getKillStreak());
   game->idle(1000);                // 1 second, in ms... game ends

   U32 timeDelta = 1000;
   TNLAssert(timeDelta < ServerGame::MaxTimeDelta, "Reduce timeDelta, please!");

   S32 iters = (S32)ceil((F32)ServerGame::LevelSwitchTime / (F32)timeDelta) - 1;

   // Idle for 4000ms more... in 1000 ms chunks because of timeDelta limitations in ServerGame
   for(S32 i = 0; i < iters; i++)
      game->idle(timeDelta);

   // New game has begun... kill streak should be reset to 0
   ASSERT_EQ(0, game->getClientInfo(0)->getKillStreak());

   delete game;
}


TEST_F(BfTest, LittleStory) 
{
   ServerGame *serverGame = newServerGame();

   GameType *gt = new GameType();    // Will be deleted in serverGame destructor
   gt->addToGame(serverGame, serverGame->getGameObjDatabase());

   ASSERT_TRUE(serverGame->isSuspended());    // ServerGame starts suspended
   serverGame->unsuspendGame(false);         

   // When adding objects to the game, use new and a pointer -- the game will 
   // delete defunct objects, so a reference will not work.
   SafePtr<Ship> ship = new Ship;
   ship->addToGame(serverGame, serverGame->getGameObjDatabase());

   ASSERT_EQ(ship->getPos(), Point(0,0));     // By default, the ship starts at 0,0
   ship->setMove(Move(0,0));
   serverGame->idle(10);
   ASSERT_EQ(ship->getPos(), Point(0,0));     // When processing move of 0,0, we expect the ship to stay put

   ship->setMove(Move(1,0));                  // Length 1 = max speed; moves stay active until replaced

   // Test that we can simulate several ticks, and the ship advances every cycle
   for(S32 i = 0; i < 20; i++)
   {
      Point prevPos = ship->getPos();
      serverGame->idle(10);                   // when i == 16 this locks up... why?
      ASSERT_NE(ship->getPos(), prevPos);    
   }

   // Note -- ship is over near (71, 0)


   // Uh oh, here comes a turret!  (will be deleted in serverGame destructor)
   Turret *t = new Turret(2, Point(71, -100), Point(0, 1));    // Turret is below the ship, pointing up
   t->addToGame(serverGame, serverGame->getGameObjDatabase());

   bool shipDeleted = false;
   for(S32 i = 0; i < 100; i++)
   {
      ship->setMove(Move(0,0));
      serverGame->idle(100);
      if(!ship)
      {
         shipDeleted = true;
         break;
      }
   }
   ASSERT_TRUE(shipDeleted);     // Ship was killed, and object was cleaned up

   delete serverGame;
}


int main(int argc, char **argv) 
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


