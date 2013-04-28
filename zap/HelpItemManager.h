
#ifndef _HELP_ITEM_MANAGER_H_
#define _HELP_ITEM_MANAGER_H_

#include "Timer.h"
#include "tnlTypes.h"
#include "tnlVector.h"

#include <string>

///////// IMPORTANT!!  Do not change the order of these items.  Do not delete any of these items.  You can update their text
/////////              or add new items, but changing the order will make a mess of the INI list that users have that records
/////////              which items have already been seen.  Instead of deleting an item, mark it as unused somehow and ignore it.
#define HELP_ITEM_TABLE                                                                                                                                             \
   HELP_TABLE_ITEM(RepairItemSpottedItem,    RepairItemTypeNumber,   Low,      ARRAYDEF({ "Repair items heal your ship.", NULL }))                                  \
   HELP_TABLE_ITEM(TestItemSpottedItem,      TestItemTypeNumber,     Low,      ARRAYDEF({ "Test Items are just bouncy objects that don't do much.", NULL }))        \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(ResourceItemSpottedItem,  ResourceItemTypeNumber, Low,      ARRAYDEF({ "If you have the Engineer module (only permitted on some levels),",       \
                                                                                          "Resource Items can be used to build things.",                            \
                                                                                          "Otherwise, they are just bouncy objects.", NULL }))                      \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(LoadoutChangedZoneItem,   LoadoutZoneTypeNumber, Immediate, ARRAYDEF({ "You've selected a new ship configuration.",                              \
                                                                                          "Find a Loadout Zone to make the changes.", NULL }))                      \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(LoadoutChangedNoZoneItem, UnknownTypeNumber,     Immediate, ARRAYDEF({ "You've selected a new ship configuration.",                              \
                                                                                          "This level has no Loadout Zones,",                                       \
                                                                                          "So you are basically screwed.", NULL }))                                 \
   HELP_TABLE_ITEM(LoadoutFinishedItem,      UnknownTypeNumber,     Immediate, ARRAYDEF({ "Loadout updated.  Good job!", NULL }))                                   \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(WelcomeItem,              UnknownTypeNumber,     Immediate, ARRAYDEF({ "Wecome to Bitfighter.  I'll help you get",                               \
                                                                                          "oriented and find your way around.",                                     \
                                                                                          "You can disable these messages in the Options menu.", NULL }))           \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(ControlsKBItem,           UnknownTypeNumber,     Paced,     ARRAYDEF({ "Move your ship with the XXX keys.",                                      \
                                                                                          "Aim and fire with the mouse.", NULL }))                                  \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(ControlsJSItem,           UnknownTypeNumber,     Paced,     ARRAYDEF({ "Move your ship with the left joystick.",                                 \
                                                                                          "Aim and fire with the right.", NULL }))                                  \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(CmdrsMapItem,             UnknownTypeNumber,     Paced,     ARRAYDEF({ "Feeling lost?  See the commander's map by pressing XXX.", NULL }))       \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(ChangeConfigItem,         UnknownTypeNumber,     Paced,     ARRAYDEF({ "You can change your ship configuration",                                 \
                                                                                          "by pressing the XXX key.", NULL }))                                      \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(GameModesItem,            UnknownTypeNumber,     Paced,     ARRAYDEF({ "Bitfighter has several game modes.",                                     \
                                                                                          "You can see the objective of the current game by pressing F2.", NULL })) \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(LowerLeftItem,            UnknownTypeNumber,     Paced,     ARRAYDEF({ "The current game type, time left, and winning score",                    \
                                                                                          "are shown in the lower-right of the screen.", NULL }))                   \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(ObjectiveArrowItem,       UnknownTypeNumber,     Paced,     ARRAYDEF({ "Objective arrows point the way to critical objects.", NULL }))           \
                                                                                                                                                                    \
   HELP_TABLE_ITEM(AddBotsItem,              UnknownTypeNumber,     Paced,     ARRAYDEF({ "Feeling lonely?  Playing with others is better, but you",                \
                                                                                          "can add bots with the /addbots command.", NULL }))                       \


using namespace TNL;
using namespace std;

namespace Zap { namespace UI {


class HelpItemManager
{

public:

enum HelpItem {
#define HELP_TABLE_ITEM(value, b, c, d) value,
   HELP_ITEM_TABLE
#undef HELP_TABLE_ITEM
   HelpItemCount
};


private:
   Vector<HelpItem> mHelpItems;
   Vector<HelpItem> mQueuedItems;
   Vector<Timer>    mHelpTimer;
   Vector<bool>     mHelpFading;
   Vector<U8>       mItemsToHighlight;

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

   void queueHelpMessage(HelpItem msg);      
   void addHelpMessage(HelpItem msg);

   void enable();
   void disable();

   void clearAlreadySeenList();

   // For loading/saving vals to the INI
   const string getAlreadySeenString() const;
   void setAlreadySeenString(const string &vals);

   const Vector<U8> *getItemsToHighlight() const;
};


} } // Nested namespace


#endif