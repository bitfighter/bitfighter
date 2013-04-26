
#ifndef _HELP_ITEM_MANAGER_H_
#define _HELP_ITEM_MANAGER_H_

#include "Timer.h"
#include "tnlTypes.h"
#include "tnlVector.h"

using namespace TNL;

namespace Zap { namespace UI {


class HelpItemManager
{

public:

enum HelpItem {
   RepairItemSpottedHelpItem,
   TestItemSpottedHelpItem,
   ResourceItemSpottedHelpItem,
   LoadoutChangedLoadoutZoneHelpItem,
   LoadoutChangedNoLoadoutZoneHelpItem,
};

private:
   Vector<HelpItem> mHelpItems;
   Vector<Timer>    mHelpTimer;
   Vector<bool>     mHelpFading;
   Vector<U8>       mItemsToHighlight;

   Timer mFloodControl;

   void buildItemsToHighlightList();

public:
   HelpItemManager();   // Constructor

   void idle(U32 timeDelta);
   void renderMessages(S32 yPos) const;

   void addHelpMessage(HelpItem msg);


   const Vector<U8> *getItemsToHighlight() const;
};


} } // Nested namespace


#endif