#include "HelpItemManager.h"

#include "BfObject.h"      // For TypeNumbers
#include "FontManager.h"
#include "Colors.h"
#include "OpenglUtils.h"
#include "RenderUtils.h"
#include "MathUtils.h"     // For min()



using namespace TNL;

namespace Zap { namespace UI {



enum Priority {
   Paced,   // These will be doled out in drips and drabs
   Low,
   High,
   Now      // Add regardless of flood control
};


static const S32 MAX_LINES = 8;     // Excluding sentinel item

struct HelpItems {
   U8 associatedItem;
   HighlightItem::Whose whose;
   Priority priority;
   const char *helpMessages[MAX_LINES + 1];
};


static HelpItems helpItems[] = {
#  define HELP_TABLE_ITEM(a, assItem, whose, priority, msgs) { assItem, HighlightItem::whose, priority, msgs},
      HELP_ITEM_TABLE
#  undef HELP_TABLE_ITEM
};


// Constructor
HelpItemManager::HelpItemManager()
{
   mFloodControl.setPeriod(10 * 1000);       // Generally, don't show items more frequently than this, in ms
   mPacedTimer.setPeriod(15 * 1000);         // How often to show a new paced message
   mInitialDelayTimer.setPeriod(4 * 1000);   // Show nothing until this timer has expired

   mDisabled = false;

#ifdef TNL_DEBUG
   mTestingCtr = 0;
   mTestingTimer.setPeriod(4000);
#endif

   clearAlreadySeenList();
}


void HelpItemManager::reset()
{
   mInitialDelayTimer.reset();               // Provide a short breather before displaying any help items
}


void HelpItemManager::idle(U32 timeDelta)
{
   mInitialDelayTimer.update(timeDelta);

   if(mInitialDelayTimer.getCurrent() > 0)
      return;

   mFloodControl.update(timeDelta);
   mPacedTimer.update(timeDelta);

   // Add queued items
   if(mPacedTimer.getCurrent() == 0 && mQueuedItems.size() > 0)
   {
      HelpItem queuedMessage = mQueuedItems[0].helpItem;
      mQueuedItems.erase(0);

      addHelpItem(queuedMessage);
      mPacedTimer.reset();
   }

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

#  ifdef TNL_DEBUG
      // This bit is for displaying our help messages one-by-one so we can see how they look on-screen
      if(mTestingTimer.getCurrent() > 0)
      {
         FontManager::pushFontContext(HelpItemContext);
         glColor(Colors::red);
         const char **messages = helpItems[mTestingCtr].helpMessages;

         // Final item in messages array will be NULL; loop until we hit that
         for(S32 j = 0; messages[j]; j++)
         {
            TNLAssert(j < MAX_LINES, "Too many lines... better increase MAX_LINES!");
            drawCenteredString(yPos, FontSize, messages[j]);
            yPos += FontSize + FontGap;
         }
         FontManager::popFontContext();
         return;
      }
#  endif

   if(mInitialDelayTimer.getCurrent() > 0)
      return
   
   FontManager::pushFontContext(HelpItemContext);

   for(S32 i = 0; i < mHelpItems.size(); i++)
   {
      F32 alpha = mHelpFading[i] ? mHelpTimer[i].getFraction() : 1;
      glColor(Colors::green, alpha);

      const char **messages = helpItems[mHelpItems[i]].helpMessages;

      // Final item in messages array will be NULL; loop until we hit that
      for(S32 j = 0; messages[j]; j++)
      {
         TNLAssert(j < MAX_LINES, "Too many lines... better increase MAX_LINES!");
         drawCenteredString(yPos, FontSize, messages[j]);
         yPos += FontSize + FontGap;
      }

      yPos += 15;    // Gap between messages
   }

   FontManager::popFontContext();
}


void HelpItemManager::queueHelpItem(HelpItem item)
{
   WeightedHelpItem weightedItem;
   weightedItem.helpItem = item;
   weightedItem.removalWeight = 0;

   mQueuedItems.push_back(weightedItem);
}


// The weight factor allows us to require several events to "vote" for removing an item before 
// it happens... basically once the weights OR to 0xFF, the item is toast.
void HelpItemManager::removeHelpItemFromQueue(HelpItem msg, U8 weight)
{
   S32 index = -1;
   for(S32 i = 0; i < mQueuedItems.size(); i++)
      if(mQueuedItems[i].helpItem == msg)
      {
         index = i;
         break;
      }

    if(index != -1)
    {
       mQueuedItems[index].removalWeight |= weight;
       if(mQueuedItems[index].removalWeight == 0xFF)
         mQueuedItems.erase(index);
    }
}


void HelpItemManager::clearAlreadySeenList()
{
   for(S32 i = 0; i < HelpItemCount; i++)
      mAlreadySeen[i] = false;
}


// Produce a string of Ys and Ns based on which messages have been seen, suitable for storing in the INI
const string HelpItemManager::getAlreadySeenString() const
{
   string s = "";

   for(S32 i = 0; i < HelpItemCount; i++)
      s += mAlreadySeen[i] ? "Y" : "N";

   return s;
}


// Takes a string; we'll mark a message as being seen every time we encounter a 'Y'
void HelpItemManager::setAlreadySeenString(const string &vals)
{
   clearAlreadySeenList();

   S32 count = MIN(vals.size(), HelpItemCount);

   for(S32 i = 0; i < count; i++)
      if(vals.at(i) == 'Y')
         mAlreadySeen[i] = true;

   // Probably should be drawn from a definition elsewhere, but some pairs of messages are dependent on one another.
   // If the first has already been shown, don't show the second
   mAlreadySeen[LoadoutFinishedItem] = mAlreadySeen[LoadoutChangedZoneItem];
}


void HelpItemManager::addHelpItem(HelpItem msg)
{
   // Nothing to do if we are disabled
   if(mDisabled)
      return;

   // Only display a message once
   if(mAlreadySeen[msg])
      return;

   // Limit the pacing of new items added -- don't add if there are queued items waiting, or priority is Now
   if((mFloodControl.getCurrent() > 0 || mQueuedItems.size() > 0) && 
      (helpItems[msg].priority != Now && helpItems[msg].priority != Paced))
      return;

   mHelpItems.push_back(msg);
   mHelpTimer.push_back(Timer(7 * 1000));    // Display time
   mHelpFading.push_back(false);

   mAlreadySeen[msg] = true;

   mFloodControl.reset();

   buildItemsToHighlightList();
}


const Vector<HighlightItem> *HelpItemManager::getItemsToHighlight() const
{
   return &mItemsToHighlight;
}


void HelpItemManager::buildItemsToHighlightList()
{
   mItemsToHighlight.clear();

   for(S32 i = 0; i < mHelpItems.size(); i++)
   {
      U8 itemType = helpItems[mHelpItems[i]].associatedItem;

      if(itemType != UnknownTypeNumber)
      {
         HighlightItem item;
         item.type = itemType;
         item.whose = helpItems[mHelpItems[i]].whose;
      
         mItemsToHighlight.push_back(item);
      }
   }
}


void HelpItemManager::enable()
{
   mDisabled = false;
}


void HelpItemManager::disable()
{
   mDisabled = true;
}



} } // Nested namespace
