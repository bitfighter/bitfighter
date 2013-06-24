//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _HELP_ITEM_MANAGER_H_
#define _HELP_ITEM_MANAGER_H_

#include "Timer.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include <string>

///////// IMPORTANT!!  Do not change the order of these items.  Do not delete any of these items.  You can update their text
/////////              or add new items, but changing the order will make a mess of the INI list that users have that records
/////////              which items have already been seen.  Instead of deleting an item, mark it as unused somehow and ignore it.
/////////              [[BindingNames]] mostly drawn from BINDING_STRINGS[] list in InputCode.cpp
/////////              See getSymbolShape() (in .cpp) for a list of other symbol substitutions you can use here.
#define HELP_ITEM_TABLE                                                                                                                                                                                        \
   HELP_TABLE_ITEM(RepairItemSpottedItem,        RepairItemTypeNumber,          Any,        Low,       ARRAYDEF({ "Repair items heal your ship.", NULL }))                                                     \
   HELP_TABLE_ITEM(TestItemSpottedItem,          TestItemTypeNumber,            Any,        Low,       ARRAYDEF({ "Test Items are just bouncy objects that don't do much.", NULL }))                           \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ResourceItemSpottedItem,      ResourceItemTypeNumber,        Any,        Low,       ARRAYDEF({ "If you have the Engineer module (only permitted on some levels),",                          \
                                                                                                                  "Resource Items can be used to build things.",                                               \
                                                                                                                  "Otherwise, they are just bouncy objects.", NULL }))                                         \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(LoadoutChangedNoZoneItem,     UnknownTypeNumber,             Any,        Now,       ARRAYDEF({ "You've selected a new ship configuration.",                                                 \
                                                                                                                  "This level has no Loadout Zones,",                                                          \
                                                                                                                  "So you are basically screwed.", NULL }))                                                    \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(LoadoutChangedZoneItem,       LoadoutZoneTypeNumber,         TorNeut,    Now,       ARRAYDEF({ "You've selected a new ship configuration.",                                                 \
                                                                                                                  "Find a Loadout Zone ([[LOADOUT_ICON]]) to make the changes.", NULL }))                      \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(LoadoutFinishedItem,          UnknownTypeNumber,             Any,        Now,       ARRAYDEF({ "Loadout updated.  Good job!", NULL }))                                                      \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(WelcomeItem,                  UnknownTypeNumber,             Any,        Now,       ARRAYDEF({ "Wecome to Bitfighter.  I'll help you get",                                                  \
                                                                                                                  "oriented and find your way around.",                                                        \
                                                                                                                  "You can disable these messages in the Options menu.", NULL }))                              \
   HELP_TABLE_ITEM(ControlsKBItem,               UnknownTypeNumber,             Any,        PacedHigh, ARRAYDEF({ "Move your ship with the [[MOVEMENT]] keys.",                                                \
                                                                                                                  "Aim and fire with the mouse.", NULL }))                                                     \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ControlsJSItem,               UnknownTypeNumber,             Any,        PacedHigh, ARRAYDEF({ "Move your ship with the left joystick.",                                                    \
                                                                                                                  "Aim and fire with the right.", NULL }))                                                     \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ModulesAndWeaponsItem,        UnknownTypeNumber,             Any,        PacedHigh, ARRAYDEF({ "Available weapons and modules are shown",                                            \
                                                                                                                  "in the upper left corner of the screen.", NULL }))                                                 \
                                                                                                                                                                                                               \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ControlsModulesItem,          UnknownTypeNumber,             Any,        PacedHigh, ARRAYDEF({ "Activate ship modules with",                                                                \
                                                                                                                  "[[MODULE_CTRL1]] and [[MODULE_CTRL2]].", NULL }))                                                            \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(CmdrsMapItem,                 UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "Feeling lost?  See the commander's map by pressing [[ShowCmdrMap]].", NULL }))              \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ChangeWeaponsItem,            UnknownTypeNumber,             Any,        PacedHigh, ARRAYDEF({ "Change weapons with [[CHANGEWEP]].", NULL }))                                               \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ChangeConfigItem,             UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "You can change your ship configuration",                                                    \
                                                                                                                  "by pressing the [[ShowLoadoutMenu]] key.", NULL }))                                         \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(GameModesItem,                UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "Bitfighter has several game modes.",                                                        \
                                                                                                                  "You can see the objective of the current game by pressing [[Mission]].", NULL }))           \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(LowerLeftItem,                UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "The current game type, time left, and winning score",                                       \
                                                                                                                  "are shown in the lower-right of the screen.", NULL }))                                      \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(ObjectiveArrowItem,           UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "Objective arrows point the way to critical objects.", NULL }))                              \
   HELP_TABLE_ITEM(AddBotsItem,                  UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "Feeling lonely?  Playing with others is better, but you",                                   \
                                                                                                                  "can add some bots from the Bots options menu.", NULL }))                                    \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(NexusSpottedItem,             NexusTypeNumber,               Any,        Low,       ARRAYDEF({ "In a Nexus Game, bring flags to the Nexus to score points.", NULL }))                       \
   HELP_TABLE_ITEM(EnergyItemSpottedItem,        EnergyItemTypeNumber,          Any,        Low,       ARRAYDEF({ "Energy Items recharge your batteries.", NULL }))                                            \
   HELP_TABLE_ITEM(FriendlyTurretSpottedItem,    TurretTypeNumber,              Team,       Low,       ARRAYDEF({ "Friendly turrets are not a threat.", NULL }))                                               \
   HELP_TABLE_ITEM(EnemyTurretSpottedItem,       TurretTypeNumber,              EorHostile, High,      ARRAYDEF({ "Enemy turrets are dangerous.", NULL }))                                                     \
   HELP_TABLE_ITEM(NeutralTurretSpottedItem,     TurretTypeNumber,              Neutral,    Low,       ARRAYDEF({ "Neutral turrets can be taken over with the Repair module.", NULL }))                        \
   HELP_TABLE_ITEM(NeutralFFSpottedItem,         ForceFieldProjectorTypeNumber, Neutral,    Low,       ARRAYDEF({ "Neutral forcefields can be taken over with the Repair module.", NULL }))                    \
   HELP_TABLE_ITEM(TeleporterSpotedItem,         TeleporterTypeNumber,          Any,        Low,       ARRAYDEF({ "Teleporters will take you places!", NULL }))                                                \
   HELP_TABLE_ITEM(GoFastSpottedItem,            SpeedZoneTypeNumber,           Any,        Low,       ARRAYDEF({ "Use GoFasts to move around quickly.", NULL }))                                              \
   HELP_TABLE_ITEM(FriendlyFFSpottedItem,        ForceFieldTypeNumber,          Any,        Low,       ARRAYDEF({ "Friendly forcefields will let you pass freely.", NULL }))                                   \
   HELP_TABLE_ITEM(FriendlyDamagedFFSpottedItem, ForceFieldProjectorTypeNumber, Any,        Low,       ARRAYDEF({ "Damaged forcefields can be repaired with the Repair module.", NULL }))                      \
   HELP_TABLE_ITEM(EnemyFFSpottedItem,           ForceFieldProjectorTypeNumber, EorHostile, Low,       ARRAYDEF({ "Disable enemy forcefields by damaging thier projector.", NULL }))                           \
   HELP_TABLE_ITEM(TryCloakItem,                 UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "Like to be sneaky?  Try the cloak module.", NULL }))                                        \
   HELP_TABLE_ITEM(ViewScoreboardItem,           UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "Who is winning?  Hit [[ShowScoreboard]] to see the scoreboard.", NULL }))                   \
   HELP_TABLE_ITEM(TryTurboItem,                 UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "You have the Boost module.  Try double-click the activation key.", NULL }))                 \
   HELP_TABLE_ITEM(TryDroppingItem,              UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "You are carrying an object.  Hit [[DropItem]] to drop it.", NULL }))                        \
   HELP_TABLE_ITEM(F1HelpItem,                   UnknownTypeNumber,             Any,        PacedLow,  ARRAYDEF({ "F1 will give you more help if you need it.", NULL }))                                       \
   HELP_TABLE_ITEM(AsteroidSpottedItem,          AsteroidTypeNumber,            Any,        High,      ARRAYDEF({ "Careful!", NULL }))                                                                         \
   HELP_TABLE_ITEM(EnemyMineSpottedItem,         MineTypeNumber,                EorHorN,    High,      ARRAYDEF({ "Enemy mines can be hard to see.  Watch out!", NULL }))                                      \
   HELP_TABLE_ITEM(FriendlyMineSpottedItem,      MineTypeNumber,                Team,       High,      ARRAYDEF({ "Friendly mines are easy to see but dangerous.", NULL }))                                    \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(FriendlySBSpottedItem,        SpyBugTypeNumber,              Team,       Low,       ARRAYDEF({ "This is a SpyBug. See enemy ships on the Cmdrs Map ([[ShowCmdrMap]]).",                     \
                                                                                                                  "Place your own with the Sensor module.", NULL }))                                           \
                                                                                                                                                                                                               \
   HELP_TABLE_ITEM(TryChattingItem,              UnknownTypeNumber,             Any,        High,      ARRAYDEF({ "Someone is sending chat messages.  Use [[TeamChat]] or [[GlobalChat]] to respond.",         \
                                                                                                                  "[[TeamChat]] sends a message to your team, [[GlobalChat]] sends one to everyone.", NULL })) \


using namespace TNL;
using namespace std;

namespace Zap { 
   

   enum HelpItem {
#define HELP_TABLE_ITEM(value, b, c, d, e) value,
   HELP_ITEM_TABLE
#undef HELP_TABLE_ITEM
   HelpItemCount,
   UnknownHelpItem
};


class InputCodeManager;
class GameSettings;
class ClientGame;

namespace UI {

struct HighlightItem
{
   enum Whose {
      Any,        // Highlight any item of this type
      Team,       // Only team's items
      TorNeut,    // Team or neutral items
      Enemy,      // Only enemy items
      Hostile,    // Only hsostile items
      EorHostile, // Enemy or hostile items
      EorHorN,    // Enemy or neutral or hostile
      Neutral     // Highilght only neutral items
   };

   U8    type;
   Whose whose; 
};


class HelpItemManager
{

private:
   struct WeightedHelpItem {
      HelpItem helpItem;
      U8       removalWeight;
   };

   Vector<HelpItem>         mHelpItems;

   Vector<WeightedHelpItem> mHighPriorityQueuedItems;
   Vector<WeightedHelpItem> mLowPriorityQueuedItems;

   Vector<Timer>            mHelpTimer;
   Vector<bool>             mHelpFading;
   Vector<HighlightItem>    mItemsToHighlight;

   InputCodeManager *mInputCodeManager;

   bool mAlreadySeen[HelpItemCount];

   Timer mPacedTimer;
   Timer mInitialDelayTimer;

   bool mEnabled;
   GameSettings *mGameSettings;

   Timer mFloodControl;

   void buildItemsToHighlightList();
   void queueHelpItem(HelpItem item);  
   void moveItemFromQueueToActiveList();

public:
   enum Priority {
      // The paced items will be doled out in drips and drabs
      PacedHigh,     // These items will always be displayed first (welcome message, basic controls)
      PacedLow,      // These items will be shown as time allows (cmdrs map, change ship config)

      // These are messages that are shown in response to events.  They get a higher priority than PacedLow.
      Low,
      High,
      Now      // Add regardless of flood control
   };

   static const S32 InitialDelayPeriod      =  4 * 1000; // Time before first help message will be displayed
   static const S32 PacedTimerPeriod        = 15 * 1000; // Rate at which paced items are displayed
   static const S32 FloodControlPeriod      = 10 * 1000; // Generally, don't show items more frequently than this, in ms
   static const S32 HelpItemDisplayPeriod   =  7 * 1000; // Time for a help item to be visible
   static const S32 HelpItemDisplayFadeTime = 500;       // Time for item to fade out


   HelpItemManager(GameSettings *settings);           // Constructor
   virtual ~HelpItemManager();

   void reset();

   void idle(U32 timeDelta);
   void renderMessages(const ClientGame *game, S32 yPos) const;

   void addInlineHelpItem(HelpItem item, bool messageCameFromQueue = false);   
   void removeHelpItemFromQueue(HelpItem item, U8 weight = 0xFF);

   void setEnabled(bool enabled);
   bool isEnabled();

   void clearAlreadySeenList();
   void saveAlreadySeenList();
   void resetInGameHelpMessages();

   // For loading/saving vals to the INI
   const string getAlreadySeenString() const;
   void setAlreadySeenString(const string &vals);

   const Vector<HighlightItem> *getItemsToHighlight() const;

   // Special accessors for tests
   const Vector<HelpItem>         *getHelpItemDisplayList() const;
   const Vector<WeightedHelpItem> *getHighPriorityQueue()   const;
   const Vector<WeightedHelpItem> *getLowPriorityQueue()    const;

   Priority getItemPriority(HelpItem item) const;
   

#ifdef TNL_DEBUG
   // For displaying items in a test capacity
   S32 mTestingCtr;
   Timer mTestingTimer;
   void debugShowNextHelpItem();
#endif
};


} } // Nested namespace


#endif
