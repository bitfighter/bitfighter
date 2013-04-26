#include "HelpItemManager.h"

#include "BfObject.h"      // For TypeNumbers
#include "FontManager.h"
#include "Colors.h"
#include "OpenglUtils.h"
#include "RenderUtils.h"



using namespace TNL;

namespace Zap { namespace UI {



static enum Priority {
   Low,
   High,
   Immediate      // Add regardless of flood control
};


static const S32 MAX_LINES = 8;     // Excluding sentinel item

struct HelpItems {
   U8 associatedItem;
   Priority priority;
   const char *helpMessages[MAX_LINES + 1];
};


static HelpItems helpItems[] = {
   { RepairItemTypeNumber,   Low,       { "Repair items heal your ship", NULL } },

   { TestItemTypeNumber,     Low,       { "Test Items are just bouncy objects that don't do much", NULL } },

   { ResourceItemTypeNumber, Low,       { "If you have the Engineer module (only permitted on some levels)", 
                                          "Resource Items can be used to build things", 
                                          "Otherwise, they are just bouncy objects", NULL } },

   { LoadoutZoneTypeNumber,  Immediate, { "You've selected a new ship configuration", 
                                          "Find a Loadout Zone to make the changes", NULL } },

   { UnknownTypeNumber,      Immediate, { "You've selected a new ship configuration", 
                                          "This level has no Loadout Zones", 
                                          "So you are basically screwed", NULL } },
};


// Constructor
HelpItemManager::HelpItemManager()
{
   mFloodControl.setPeriod(10 * 1000);      // Generally, don't show items more frequently than this, in ms
}


void HelpItemManager::idle(U32 timeDelta)
{
   mFloodControl.update(timeDelta);

   for(S32 i = 0; i < mHelpTimer.size(); i++)
      if(mHelpTimer[i].update(timeDelta))
      {
         if(mHelpFading[i])
         {
            mHelpItems.erase(i);
            mHelpFading.erase(i);
            mHelpTimer.erase(i);

            buildItemsToHighlightList();
         }
         else
         {
            mHelpFading[i] = true;
            mHelpTimer[i].reset(500);
         }
      }
}


void HelpItemManager::renderMessages(S32 yPos) const
{
   static const S32 FontSize = 18;
   static const S32 FontGap  = 6;

   FontManager::pushFontContext(FontManager::HelpItemContext);

   for(S32 i = 0; i < mHelpItems.size(); i++)
   {
      F32 alpha = mHelpFading[i] ? mHelpTimer[i].getFraction() : 1;
      glColor(Colors::green, alpha);

      const char **messages = helpItems[mHelpItems[i]].helpMessages;

      // Final item in messages array will be NULL; iterate until we hit that
      for(S32 j = 0; messages[j]; j++)
      {
         TNLAssert(j < MAX_LINES, "Too many lines... better increase MAX_LINES!");
         drawCenteredString(yPos, FontSize, messages[j]);
         yPos += FontSize + FontGap;
      }

      yPos += 10;    // Gap between messages
   }

   FontManager::popFontContext();
}


void HelpItemManager::addHelpMessage(HelpItem msg)
{
   // Make sure we don't end up with a duplicate message -- should we renew the timer in this instance?
   if(mHelpItems.contains(msg))
      return;

   // Limit the pacing of new items added -- unless item has immediate priority
   if(mFloodControl.getCurrent() > 0 && helpItems[msg].priority != Immediate)
      return;

   mHelpItems.push_back(msg);
   mHelpTimer.push_back(Timer(10000));    // Display time
   mHelpFading.push_back(false);

   mFloodControl.reset();

   buildItemsToHighlightList();
}


const Vector<U8> *HelpItemManager::getItemsToHighlight() const
{
   return &mItemsToHighlight;
}


void HelpItemManager::buildItemsToHighlightList()
{
   mItemsToHighlight.clear();

   for(S32 i = 0; i < mHelpItems.size(); i++)
   {
      U8 assItem = helpItems[mHelpItems[i]].associatedItem;

      if(assItem != UnknownTypeNumber)
         mItemsToHighlight.push_back(assItem);
   }
}


} } // Nested namespace