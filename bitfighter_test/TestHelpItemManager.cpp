//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "TestUtils.h"
#include "gtest/gtest.h"
#include "HelpItemManager.h"
#include "ClientGame.h"
#include "gameType.h"

namespace Zap
{

class HelpItemManagerTest : public testing::Test
{
public:
   ClientGame* game;
   GameType* gameType;
   HelpItem helpItem;
   UI::HelpItemManager himgr;

   HelpItemManagerTest()
   : game(newClientGame()), himgr(UI::HelpItemManager(game->getSettings()))
   {
      // Need to add a gameType because gameType is where the game timer is managed
      GameType *gameType = new GameType();    // Will be deleted in game destructor
      gameType->addToGame(game, game->getGameObjDatabase());
   }


   ~HelpItemManagerTest() {
      delete game;
   }


   void checkQueues(S32 highSize, S32 lowSize, S32 displaySize, HelpItem displayItem = UnknownHelpItem)
   {
      // Check that queue sizes match what we specified
      ASSERT_EQ(himgr.getHighPriorityQueue()->size(), highSize);
      ASSERT_EQ(himgr.getLowPriorityQueue()->size(), lowSize);
      ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), displaySize);

      // Check if displayItem is being displayed, unless displayItem is UnknownHelpItem
      if(displayItem != UnknownHelpItem && himgr.getHelpItemDisplayList()->size() > 0)
         ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), displayItem);
   }


   // Full cycle is PacedTimerPeriod; that is, every PacedTimerPeriod ms, a new
   // item is displayed Within that cycle, and item is displayed for
   // HelpItemDisplayPeriod, then faded for getRollupPeriod, at which point it
   // is expired and removed from the screen.  Then we must wait until the
   // remainder of PacedTimerPeriod has elapsed before a new item will be shown.
   // The following methods try to makes sense of that.
   S32 idleUntilItemExpiredMinusOne()
   {
      S32 lastRollupPeriod = himgr.getRollupPeriod(0);
      himgr.idle(himgr.HelpItemDisplayPeriod,  game);   // Can't combine these periods... needs to be two different idle cycles
      himgr.idle(lastRollupPeriod - 1, game);   // First cycle to enter fade mode, second to exhaust fade timer
      return lastRollupPeriod;
   }


   // Idles HelpItemDisplayPeriod + getRollupPeriod(), long enough for a freshly
   // displayed item to disappear
   S32 idleUntilItemExpired()
   {
      S32 lastRollupPeriod = idleUntilItemExpiredMinusOne();
      himgr.idle(1, game);
      return lastRollupPeriod;
   }


   void idleRemainderOfFullCycleMinusOne(S32 lastRollupPeriod)
   {
      himgr.idle(himgr.PacedTimerPeriod - himgr.HelpItemDisplayPeriod - lastRollupPeriod - 1, game);
   }


   // Assumes we've idled HelpItemDisplayPeriod + getRollupPeriod(), idles remainder of PacedTimerPeriod
   void idleRemainderOfFullCycle(S32 lastRollupPeriod)
   {
      idleRemainderOfFullCycleMinusOne(lastRollupPeriod);
      himgr.idle(1, game);
   }


   void idleFullCycleMinusOne()
   {
      S32 lastRollupPeriod = idleUntilItemExpired();
      idleRemainderOfFullCycleMinusOne(lastRollupPeriod);
   }


   // Idles full PacedTimerPeriod, broken into strategic parts
   void idleFullCycle()
   {
      idleFullCycleMinusOne();
      himgr.idle(1, game);
   }
};


TEST_F(HelpItemManagerTest, INIStorage)
{
   // Since different items take different times to rollup when their display
   // period is over, and since we can only determine that time period when
   // items are being displayed, we will grab that value when we can and store
   // it for future use.  We'll always store it in lastRollupPeriod.
   HelpItem helpItem = NexusSpottedItem;

   ASSERT_EQ(himgr.getAlreadySeenString()[helpItem], 'N');  // Item not marked as seen originally

   himgr.addInlineHelpItem(helpItem);                       // Add up a help item -- should not get added during initial delay period
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify no help item is being displayed

   himgr.idle(himgr.InitialDelayPeriod, game);              // Idle out of initial delay period

   himgr.addInlineHelpItem(helpItem);                       // Requeue helpItem -- initial delay period has expired, so should get added
   ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), helpItem);

   ASSERT_EQ(himgr.getAlreadySeenString()[helpItem], 'Y');  // Item marked as seen
   ASSERT_EQ(game->getSettings()->getIniSettings()->mSettings.getVal<string>("HelpItemsAlreadySeenList"), himgr.getAlreadySeenString());  // Verify changes made it to the INI

   idleUntilItemExpired();

   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify help item is no longer displayed
   himgr.addInlineHelpItem(TeleporterSpotedItem);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Still in flood control period, new item not added

   himgr.idle(himgr.FloodControlPeriod, game);
   himgr.addInlineHelpItem(helpItem);                       // We've already added this item, so it should not be displayed again
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);    // Verify help item is not displayed again
   himgr.addInlineHelpItem(TeleporterSpotedItem);           // Now, new item should be added (we tried adding this above, and it didn't go)
   ASSERT_EQ(himgr.getHelpItemDisplayList()->get(0), TeleporterSpotedItem);   // Verify help item is now visible
}

TEST_F(HelpItemManagerTest, priority)
{
   S32 lastRollupPeriod;
   HelpItem highPriorityItem = ControlsKBItem;
   HelpItem lowPriorityItem  = CmdrsMapItem;
   HelpItem gameStartPriorityItem = NexGameStartItem;

   ASSERT_EQ(himgr.getItemPriority(highPriorityItem),      UI::HelpItemManager::PacedHigh);    // Prove that these items
   ASSERT_EQ(himgr.getItemPriority(lowPriorityItem),       UI::HelpItemManager::PacedLow);     // have the expected priority
   ASSERT_EQ(himgr.getItemPriority(gameStartPriorityItem), UI::HelpItemManager::GameStart);     

   himgr.addInlineHelpItem(lowPriorityItem);    // Queue both items up
   himgr.addInlineHelpItem(highPriorityItem);

   // Verify that both have been queued -- one item in each queue, and there are no items in the display list
   checkQueues(1, 1, 0);

   // Now queue the gameStartPriorityItem... it should not get queued, and things should remain as they were
   himgr.addInlineHelpItem(gameStartPriorityItem);
   checkQueues(1, 1, 0);

   himgr.idle(himgr.InitialDelayPeriod - 1, game);
   ASSERT_EQ(himgr.getHelpItemDisplayList()->size(), 0);

   himgr.idle(1, game);    // InitialDelayPeriod has expired

   // Verify that our high priority item has been moved to the active display list
   checkQueues(0, 1, 1, highPriorityItem);

   S32 rollupPeriod;
   rollupPeriod = himgr.getRollupPeriod(0);
   himgr.idle(himgr.HelpItemDisplayPeriod, game); himgr.idle(rollupPeriod - 1, game);  // (can't combine)

   // Verify nothing has changed
   checkQueues(0, 1, 1, highPriorityItem);
   
   himgr.idle(1, game);    // Now, help item should no longer be displayed
   checkQueues(0, 1, 0);

   himgr.idle(himgr.PacedTimerPeriod - himgr.HelpItemDisplayPeriod - rollupPeriod - 1, game); // Idle to cusp of pacedTimer expiring
   // Verify nothing has changed
   checkQueues(0, 1, 0);
   himgr.idle(1, game);    // pacedTimer has expired, a new pacedItem will be added to the display list

   // Second message should now be moved from the lowPriorityQueue to the active display list, and will be visible
   lastRollupPeriod = himgr.getRollupPeriod(0);
   checkQueues(0, 0, 1, lowPriorityItem);

   // Idle until list is clear
   idleUntilItemExpired();
   checkQueues(0, 0, 0);       // No items displayed
   idleRemainderOfFullCycle(lastRollupPeriod);

   // Try adding our gameStartPriorityItem now... since there is nothing in the high priority queue, it should add fine
   himgr.addInlineHelpItem(gameStartPriorityItem);
   checkQueues(1, 0, 0);
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(0, 0, 1);       // No items displayed
   idleFullCycle();
   checkQueues(0, 0, 0);       // No items displayed

   // Make sure an item in the low priority queue won't impede the addition of a gameStartPriorityItem
   himgr.clearAlreadySeenList();    // Allows us to add these same items again
   himgr.addInlineHelpItem(lowPriorityItem);
   checkQueues(0, 1, 0);
   himgr.addInlineHelpItem(gameStartPriorityItem);
   checkQueues(1, 1, 0);
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(0, 1, 1, gameStartPriorityItem);       // gameStartPriority item displayed
   idleFullCycle();
   checkQueues(0, 0, 1);
   idleFullCycle();
   checkQueues(0, 0, 0);
}


// Verify bug with two high priority paced items preventing the first from being displayed
TEST_F(HelpItemManagerTest, highPriorityClobberingBug)
{
   // Verify they have HighPaced priority, then queue them up
   ASSERT_EQ(himgr.getItemPriority(ControlsKBItem),      UI::HelpItemManager::PacedHigh);
   ASSERT_EQ(himgr.getItemPriority(ControlsModulesItem), UI::HelpItemManager::PacedHigh);
   himgr.addInlineHelpItem(ControlsKBItem);
   himgr.addInlineHelpItem(ControlsModulesItem);
   checkQueues(2, 0, 0);                       // Two high priority items queued, none displayed

   himgr.idle(himgr.InitialDelayPeriod, game);        // Wait past the intial delay period; first item will be displayed
   checkQueues(1, 0, 1, ControlsKBItem);       // One item queued, one displayed

   idleUntilItemExpiredMinusOne();
   checkQueues(1, 0, 1, ControlsKBItem);       // One item queued, one displayed

   S32 lastRollupPeriod = himgr.getRollupPeriod(0);
   himgr.idle(1, game);
   idleRemainderOfFullCycleMinusOne(lastRollupPeriod);
   checkQueues(1, 0, 0);
   himgr.idle(1, game);
   checkQueues(0, 0, 1, ControlsModulesItem);  // No items queued, one displayed

   lastRollupPeriod = himgr.getRollupPeriod(0);

   idleUntilItemExpired();                 // Clear the decks
   checkQueues(0, 0, 0);
   checkQueues(0, 0, 0);

   himgr.clearAlreadySeenList();    // Allows us to add the same items again

   // Check the increasingly complex logic inside moveItemFromQueueToActiveList()
   // We already know that the logic with high-priority and low-priority lists is working under ordinary circumstances
   // So let's ensure it is working for the special situation of AddBotsItem, which should be ignored if there are bots in the game 
   // Remember that we are in mid-cycle, so we need to finish the cycle* before new items are displayed
   himgr.addInlineHelpItem(AddBotsItem);
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(0, 2, 0);  // Both items should be queued, but none displayed

   idleRemainderOfFullCycle(lastRollupPeriod); // Cycle is completed, ready for a new item
   checkQueues(0, 1, 1, AddBotsItem);                // No bots are in game ==> AddBotsItem should be displayed

   // Reset
   idleFullCycle();
   checkQueues(0, 0, 1, CmdrsMapItem);               // One item displayed, none in the queues
   idleFullCycleMinusOne();
   checkQueues(0, 0, 0);                             // No items dispalyed, queues are empty

   himgr.clearAlreadySeenList();    // Allows us to add the same items again

   // Add the same two items again
   himgr.addInlineHelpItem(AddBotsItem);
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(0, 2, 0);     // Both items should be queued, none yet displayed

   // Check that adding a duplicate item does not affect the queues (dupe should be ignored)
   himgr.addInlineHelpItem(CmdrsMapItem);
   checkQueues(0, 2, 0);     // Still 2 items displayed

   // Create a ClientInfo for a robot -- it will be deleted by ClientGame destructor
   RemoteClientInfo *clientInfo = new RemoteClientInfo(game, "Robot", true, 0, 0, 0, true, ClientInfo::RoleNone, false, false);
   clientInfo->setTeamIndex(0);        // Assign it to our team
   game->addToClientList(clientInfo);  // Add it to the game
   ASSERT_EQ(game->getBotCount(), 1);  // Confirm there is a bot in the game

   himgr.idle(1, game);
   checkQueues(0, 1, 1, CmdrsMapItem); // With bot in game, AddBotsItem should be bypassed, should see CmdrsMapItem

   idleUntilItemExpired();
   checkQueues(0, 1, 0);              // Bot help still not showing
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(0, 1, 0);              // Bot help still not showing
   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);
   checkQueues(0, 1, 0);              // And again!

   game->removeFromClientList(clientInfo);   // Remove the bot
   ASSERT_EQ(game->getBotCount(), 0);        // Confirm no bots in the game

   himgr.idle(UI::HelpItemManager::PacedTimerPeriod, game);      // Not exact, just a big chunk of time
   checkQueues(0, 0, 1, AddBotsItem); // With no bot, AddBotsItem will be displayed, though not necessarily immediately
}


};