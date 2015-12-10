//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "ChatMessageDisplayer.h"
#include "GameManager.h"
#include "gameType.h"
#include "Level.h"
#include "ServerGame.h"
#include "ship.h"
#include "stringUtils.h"
#include "UIGame.h"
#include "UIManager.h"

#include "Colors.h"
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
      clientGames->get(i)->getLevel()->findObjects(TeleporterTypeNumber, fillVector);
      EXPECT_EQ(1, fillVector.size()) << "Expected a teleporter!";
   }
}


// Do this in a macro to make it easier to call private members of tested class
#define CHECK_DISPLAY_COUNTS(cmd, msgsVisibleWhenNotComposing, msgsVisibleWhenComposing, isScrolling)       \
   EXPECT_EQ(msgsVisibleWhenNotComposing, cmd.getCountOfMessagesToDisplay(0, false));                       \
   EXPECT_EQ(msgsVisibleWhenComposing,    cmd.getCountOfMessagesToDisplay(1, false));                       \
   cmd.toggleDisplayMode();                                                                                 \
   EXPECT_EQ(msgsVisibleWhenComposing,    cmd.getCountOfMessagesToDisplay(0, false));                       \
   EXPECT_EQ(msgsVisibleWhenComposing,    cmd.getCountOfMessagesToDisplay(1, false));                       \
   cmd.toggleDisplayMode();                                                                                 \
   EXPECT_EQ(msgsVisibleWhenComposing,    cmd.getCountOfMessagesToDisplay(0, false));                       \
   EXPECT_EQ(msgsVisibleWhenComposing,    cmd.getCountOfMessagesToDisplay(1, false));                       \
   cmd.toggleDisplayMode();                                                                                 \
   EXPECT_EQ(cmd.mChatScrollTimer.getCurrent() > 0, isScrolling)                                            \
   // EXPECT_EQ(cmd.isScrolling(), isScrolling)   \
   // For some reason the line above causes an error...


// Test coverage here is incomplete
TEST(GameUserInterfaceTest, ChatMessageDisplayer)
{
   //ClientGame *game, S32 msgCount, bool topDown, S32 wrapWidth, S32 fontSize, S32 fontGap
   GamePair gamePair;
   ClientGame *game = gamePair.getClient(0);

   ChatMessageDisplayer cmd(game, 5, true, 500, 7, 3);

   ASSERT_EQ(cmd.mDisplayMode, ChatMessageDisplayer::ShortTimeout) << "This should be the mode we start in";

   // Verify that cycling modes works as expected... nothing wrong if this changes, but tests will need to be rewritten
   ASSERT_EQ(cmd.mDisplayMode, ChatMessageDisplayer::ShortTimeout)   << "This should be the mode we start in";
   ASSERT_EQ(3, ChatMessageDisplayer::MessageDisplayModes)           << "Expect 3 modes; if not, test needs to be rewritten";
   cmd.toggleDisplayMode(); cmd.toggleDisplayMode(); cmd.toggleDisplayMode(); 
   ASSERT_EQ(cmd.mDisplayMode, ChatMessageDisplayer::ShortTimeout)   << "Should be back to starting mode";


   // No messages yet...
   EXPECT_EQ(0, cmd.getCountOfMessagesToDisplay(0, false));
   EXPECT_EQ(0, cmd.getCountOfMessagesToDisplay(1, false));
   EXPECT_FALSE(cmd.isScrolling());

   // Simple message lifecycle
   cmd.onChatMessageReceived(Colors::red, "Message 1");
   EXPECT_EQ(1, cmd.getCountOfMessagesToDisplay(0, false));
   EXPECT_FALSE(cmd.isScrolling());
   cmd.idle(ChatMessageDisplayer::MESSAGE_EXPIRE_TIME, false);    // Message expires, and is just starting to scroll
   EXPECT_EQ(1, cmd.getCountOfMessagesToDisplay(0, false));
   EXPECT_TRUE(cmd.isScrolling());
   cmd.idle(ChatMessageDisplayer::SCROLL_TIME - 1, false);
   EXPECT_EQ(1, cmd.getCountOfMessagesToDisplay(0, false));
   EXPECT_TRUE(cmd.isScrolling());
   cmd.idle(1, false);
   EXPECT_EQ(0, cmd.getCountOfMessagesToDisplay(0, false));
   EXPECT_FALSE(cmd.isScrolling());

   EXPECT_EQ(1, cmd.getCountOfMessagesToDisplay(0, true)) << "Started composing, should see old messages";
   EXPECT_EQ(1, cmd.getCountOfMessagesToDisplay(1, false));
   EXPECT_EQ(1, cmd.getCountOfMessagesToDisplay(1, true));
   EXPECT_EQ(0, cmd.getCountOfMessagesToDisplay(0, false));

   // Reset
   cmd.reset();
   EXPECT_EQ(0, cmd.getCountOfMessagesToDisplay(0, true));
   EXPECT_EQ(0, cmd.getCountOfMessagesToDisplay(1, false));
   EXPECT_FALSE(cmd.isScrolling());

   /////
   // Full compliment of messages, time such that only one will be scrolling at a time
   ASSERT_TRUE(ChatMessageDisplayer::MESSAGE_EXPIRE_TIME > 5 * (ChatMessageDisplayer::SCROLL_TIME + 1)) << "Assumption required for test, nothing actually wrong with this condition";
   for(S32 i = 0; i < 5; i++)
   {
      cmd.onChatMessageReceived(Colors::red, "Message " + itos(i + 1));
      cmd.idle(ChatMessageDisplayer::SCROLL_TIME + 1, false);
   }

   CHECK_DISPLAY_COUNTS(cmd, 5, 5, false);

   // Let first expire
   cmd.idle(ChatMessageDisplayer::MESSAGE_EXPIRE_TIME - 5 * ChatMessageDisplayer::SCROLL_TIME - 5, false);
   CHECK_DISPLAY_COUNTS(cmd, 5, 5, true);

   cmd.idle(ChatMessageDisplayer::SCROLL_TIME - 1, false);
   CHECK_DISPLAY_COUNTS(cmd, 5, 5, true);

   // First stops scrolling
   cmd.idle(1, false);
   CHECK_DISPLAY_COUNTS(cmd, 4, 5, false);

   // Second starts scrolling
   cmd.idle(1, false);
   CHECK_DISPLAY_COUNTS(cmd, 4, 5, true);
   
   cmd.idle(ChatMessageDisplayer::SCROLL_TIME - 1, false);
   CHECK_DISPLAY_COUNTS(cmd, 4, 5, true);

   // Second stops scrolling
   cmd.idle(1, false);
   CHECK_DISPLAY_COUNTS(cmd, 3, 5, false);

   // Third starts scrolling
   cmd.idle(1, false);
   CHECK_DISPLAY_COUNTS(cmd, 3, 5, true);
   cmd.idle(ChatMessageDisplayer::SCROLL_TIME - 1, false);
   CHECK_DISPLAY_COUNTS(cmd, 3, 5, true);
   // Third stops scrolling
   cmd.idle(1, false);
   CHECK_DISPLAY_COUNTS(cmd, 2, 5, false);

   // Long time passing... all should be expired
   for(S32 i = 0; i < 100; i++)
     cmd.idle(ChatMessageDisplayer::SCROLL_TIME + ChatMessageDisplayer::MESSAGE_EXPIRE_TIME, false);
   CHECK_DISPLAY_COUNTS(cmd, 0, 5, false);

   cmd.reset();

   ///// More than a full compliment of messages
   ASSERT_TRUE(ChatMessageDisplayer::MESSAGE_EXPIRE_TIME > 8 * (ChatMessageDisplayer::SCROLL_TIME + 1)) << "Assumption required for test, nothing actually wrong with this condition";
   for(S32 i = 0; i < 8; i++)
   {
      cmd.onChatMessageReceived(Colors::red, "Message " + itos(i + 1));
      cmd.idle(ChatMessageDisplayer::SCROLL_TIME + 1, false);
   }

   // Let them all expire
   for(S32 i = 0; i < 100; i++)
     cmd.idle(ChatMessageDisplayer::SCROLL_TIME + ChatMessageDisplayer::MESSAGE_EXPIRE_TIME, false);

   ASSERT_EQ(0, cmd.getCountOfMessagesToDisplay(0, false)) << "No messages should be visible after all have expired";
}


};
