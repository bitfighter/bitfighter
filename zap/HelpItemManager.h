
#ifndef _HELP_ITEM_MANAGER_H_
#define _HELP_ITEM_MANAGER_H_

#include "Timer.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include <string>

///////// IMPORTANT!!  Do not change the order of these items.  Do not delete any of these items.  You can update their text
/////////              or add new items, but changing the order will make a mess of the INI list that users have that records
/////////              which items have already been seen.  Instead of deleting an item, mark it as unused somehow and ignore it.
#define HELP_ITEM_TABLE                                                                                                                                                \
   HELP_TABLE_ITEM(RepairItemSpottedItem,    RepairItemTypeNumber,   Any,    Low,  ARRAYDEF({ "Repair items heal your ship.", NULL }))                                   \
   HELP_TABLE_ITEM(TestItemSpottedItem,      TestItemTypeNumber,     Any,    Low,  ARRAYDEF({ "Test Items are just bouncy objects that don't do much.", NULL }))         \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ResourceItemSpottedItem,  ResourceItemTypeNumber, Any,    Low,  ARRAYDEF({ "If you have the Engineer module (only permitted on some levels),",        \
                                                                                              "Resource Items can be used to build things.",                             \
                                                                                              "Otherwise, they are just bouncy objects.", NULL }))                       \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(LoadoutChangedNoZoneItem, UnknownTypeNumber,      Any,    Now,   ARRAYDEF({ "You've selected a new ship configuration.",                              \
                                                                                               "This level has no Loadout Zones,",                                       \
                                                                                               "So you are basically screwed.", NULL }))                                 \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(LoadoutChangedZoneItem,   LoadoutZoneTypeNumber, TorNeut, Now,   ARRAYDEF({ "You've selected a new ship configuration.",                              \
                                                                                               "Find a Loadout Zone to make the changes.", NULL }))                      \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(LoadoutFinishedItem,      UnknownTypeNumber,      Any,    Now,   ARRAYDEF({ "Loadout updated.  Good job!", NULL }))                                   \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(WelcomeItem,              UnknownTypeNumber,      Any,    Now,   ARRAYDEF({ "Wecome to Bitfighter.  I'll help you get",                               \
                                                                                               "oriented and find your way around.",                                     \
                                                                                               "You can disable these messages in the Options menu.", NULL }))           \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ControlsKBItem,           UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Move your ship with the XXX keys.",                                      \
                                                                                               "Aim and fire with the mouse.", NULL }))                                  \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ControlsJSItem,           UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Move your ship with the left joystick.",                                 \
                                                                                               "Aim and fire with the right.", NULL }))                                  \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ControlsModulesItem,      UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Activate ship modules with",                                             \
                                                                                               "the XXX keys/buttons.", NULL }))                                         \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(CmdrsMapItem,             UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Feeling lost?  See the commander's map by pressing XXX.", NULL }))       \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ChangeWeaponsItem,        UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Change weapons with the XXXX key.", NULL }))                             \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ChangeConfigItem,         UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "You can change your ship configuration",                                 \
                                                                                               "by pressing the XXX key.", NULL }))                                      \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(GameModesItem,            UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Bitfighter has several game modes.",                                     \
                                                                                               "You can see the objective of the current game by pressing F2.", NULL })) \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(LowerLeftItem,            UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "The current game type, time left, and winning score",                    \
                                                                                               "are shown in the lower-right of the screen.", NULL }))                   \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(ObjectiveArrowItem,       UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Objective arrows point the way to critical objects.", NULL }))           \
                                                                                                                                                                         \
   HELP_TABLE_ITEM(AddBotsItem,              UnknownTypeNumber,      Any,    Paced, ARRAYDEF({ "Feeling lonely?  Playing with others is better, but you",                \
                                                                                               "can add bots with the /addbots command.", NULL }))                       \

//In a Nexus Game, bring flags to the Nexus to score points.
//Energy Items recharge your batteries.
//Friendly turrets are not a threat.
//Enemy turrets are dangerous.
//Neutral turrets can be "converted" with the Repair module.
//Teleporters will take you places!
//Use GoFasts to move around quickly.
//Disable enemy forcefields by damaging thier projector.
//Friendly forcefields will let you pass freely.
//Like to be sneaky?  Try the cloak module.
//Want to know who's winning?  Hit xxxx to see the scoreboard.
//You've got the Boost module.  Try double-click the activation key.
//You are carrying an object.  Hit XXX to drop it.
//Someone is sending chat messages.  Use XXX or XXX to respond.
//   XXX sends a message to your team, YYY sends one to everyone.
//F1 will give you more help if you need it.
//Ever play Asteroids?  These are the real deal.   (or maybe just "Careful!")
//Enemy mines can be hard to see.
//Friendly mines are visible but dangerous.
//This is a SpyBug. You can see enemy ships on the Cmdrs Map ([xXX]).
//   Place your own with the Sensor module.




using namespace TNL;
using namespace std;

namespace Zap { 
   

   enum HelpItem {
#define HELP_TABLE_ITEM(value, b, c, d, e) value,
   HELP_ITEM_TABLE
#undef HELP_TABLE_ITEM
   HelpItemCount
};

namespace UI {


struct HighlightItem
{
   enum Whose {
      Any,     // Highlight any item of this type
      Team,    // Highlight only team's items
      TorNeut, // Highlight team or neutral items
      Enemy,   // Highilght only enemy items
      Hostile, // Highilght only hsostile items
      Neutral  // Highilght only neutral items
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

   Vector<WeightedHelpItem> mQueuedItems;
   Vector<Timer>            mHelpTimer;
   Vector<bool>             mHelpFading;
   Vector<HighlightItem>    mItemsToHighlight;

   bool mAlreadySeen[HelpItemCount];

   Timer mPacedTimer;
   Timer mInitialDelayTimer;

   bool mDisabled;

   Timer mFloodControl;

   void buildItemsToHighlightList();

public:
   HelpItemManager();   // Constructor

   void reset();

   void idle(U32 timeDelta);
   void renderMessages(S32 yPos) const;

   void queueHelpItem(HelpItem item);  
   void removeHelpItemFromQueue(HelpItem item, U8 weight = 0xFF);
   void addHelpItem(HelpItem item);

   void enable();
   void disable();

   void clearAlreadySeenList();

   // For loading/saving vals to the INI
   const string getAlreadySeenString() const;
   void setAlreadySeenString(const string &vals);

   const Vector<HighlightItem> *getItemsToHighlight() const;
};


} } // Nested namespace


#endif