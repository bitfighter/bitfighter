//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIGame.h"
#include "GameManager.h"
#include "UIManager.h"
#include "ServerGame.h"
#include "gameType.h"
#include "stringUtils.h"
#include "Level.h"

#include "TestUtils.h"
#include "EventKeyDefs.h"
#include "LevelFilesForTesting.h"

#include "gtest/gtest.h"

namespace Zap
{

TEST(GameUserInterfaceTest, Engineer)
{
   InputCodeManager::initializeKeyNames();  

   GamePair gamePair(getLevelCodeForTestingEngineer1(), 3); // See def for description of level
   ServerGame *serverGame                  = GameManager::getServerGame();
   const Vector<ClientGame *> *clientGames = GameManager::getClientGames();
   GameSettings *clientSettings            = clientGames->first()->getSettings();
   GameUserInterface *gameUI               = clientGames->first()->getUIManager()->getUI<GameUserInterface>();

   DEFINE_KEYS_AND_EVENTS(clientSettings);

   // Idle for a while, let things settle
   GamePair::idle(10, 5);

   // Verify that engineer is enabled
   ASSERT_TRUE(serverGame->getGameType()->isEngineerEnabled()) << "Engineer should be enabled on server!";

   for(S32 i = 0; i < clientGames->size(); i++)
   {
      SCOPED_TRACE("i = " + itos(i));
      ASSERT_TRUE(clientGames->get(i)->getGameType())                      << "Clients should have a GameType by now!";
      ASSERT_TRUE(clientGames->get(i)->getGameType()->isEngineerEnabled()) << "Engineer mode not propagating to clients!";
   }

   ASSERT_TRUE(serverGame->getClientInfo(0)->getShip()->isInZone(LoadoutZoneTypeNumber)); // Check level is as we expected

   // Add engineer to current loadout
   ASSERT_FALSE(gameUI->isHelperActive(HelperMenu::LoadoutHelperType));
   gameUI->activateHelper(HelperMenu::LoadoutHelperType);
   ASSERT_TRUE(gameUI->isHelperActive(HelperMenu::LoadoutHelperType));

   gameUI->onKeyDown(LOADOUT_KEY_ENGR);      gameUI->onKeyDown(LOADOUT_KEY_REPAIR);                                           // First 2 modules...
   gameUI->onKeyDown(LOADOUT_KEY_PHASER);    gameUI->onKeyDown(LOADOUT_KEY_BOUNCE);    gameUI->onKeyDown(LOADOUT_KEY_TRIPLE); // ...then 3 weapons

   // On this level, the ship spawn is inside a loadout zone, so loadout should take effect immediately
   GamePair::idle(10, 5);
   ASSERT_EQ("Engineer,Repair,Phaser,Bouncer,Triple", serverGame->getClientInfo(0)->getShip()->getLoadoutString());
   for(S32 i = 0; i < clientGames->size(); i++)
   {
      SCOPED_TRACE("i = " + itos(i));
      ASSERT_EQ("Engineer,Repair,Phaser,Bouncer,Triple", clientGames->get(0)->getLocalPlayerShip()->getLoadoutString());
   }

   // Fly down to pick up resource item -- to get flying to work, need to create events at a very basic level
   ClientGame *clientGame = clientGames->get(0);
   Point startPos = clientGame->getLocalPlayerShip()->getActualPos();
   Event::onEvent(clientGame, &EventDownPressed);
   GamePair::idle(100, 5);
   Event::onEvent(clientGame, &EventDownReleased);
   Point endPos = clientGame->getLocalPlayerShip()->getActualPos();
   ASSERT_TRUE(startPos.distSquared(endPos) > 0) << "Ship did not move!!";
   for(S32 i = 0; i < clientGames->size(); i++)
   {
      SCOPED_TRACE("i = " + itos(i));
      ASSERT_TRUE(clientGames->get(0)->getLocalPlayerShip()->isCarryingItem(ResourceItemTypeNumber));
   }

   // Time to engineer!
   gameUI->onKeyDown(KEY_MOD1);
   ASSERT_TRUE(gameUI->isHelperActive(HelperMenu::EngineerHelperType)) << "Expect engineer helper menu to be active!";

   InputCode key = EngineerHelper::getInputCodeForOption(EngineeredTeleporterEntrance, true);
   gameUI->onKeyDown(key);
   ASSERT_FALSE(static_cast<const EngineerHelper *>(gameUI->getActiveHelper())->isMenuBeingDisplayed());
   gameUI->onKeyDown(KEY_MOD1);     // Place entrance
   gameUI->onKeyDown(KEY_MOD1);     // Place exit
   GamePair::idle(100, 5);           // Let things mellow

   for(S32 i = 0; i < clientGames->size(); i++)
   {
      SCOPED_TRACE("i = " + itos(i));
      Vector<DatabaseObject *> fillVector;
      clientGames->get(i)->getGameObjDatabase()->findObjects(TeleporterTypeNumber, fillVector);
      EXPECT_EQ(1, fillVector.size()) << "Expected a teleporter!";
   }
}

};
