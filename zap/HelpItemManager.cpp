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
   Paced,         // These will be doled out in drips and drabs
   Low,
   High,
   Immediate      // Add regardless of flood control
};

enum Whose {



static const S32 MAX_LINES = 8;     // Excluding sentinel item

struct HelpItems {
   U8 associatedItem;
   Priority priority;
   const char *helpMessages[MAX_LINES + 1];
};


static HelpItems helpItems[] = {
#define HELP_TABLE_ITEM(a, assItem, priority, msgs) { assItem, priority, msgs},
      HELP_ITEM_TABLE
   #undef HELP_TABLE_ITEM
};


// Constructor
HelpItemManager::HelpItemManager()
{
   mFloodControl.setPeriod(10 * 1000);       // Generally, don't show items more frequently than this, in ms
   mPacedTimer.setPeriod(15 * 1000);         // How often to show a new paced message
   mInitialDelayTimer.setPeriod(4 * 1000);   // Show nothing until this timer has expired

   mDisabled = false;

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
      HelpItem queuedMessage = mQueuedItems[0];
      mQueuedItems.erase(0);

      addHelpMessage(queuedMessage);
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

   if(mInitialDelayTimer.getCurrent() > 0)
      return;

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

      yPos += 15;    // Gap between messages
   }

   FontManager::popFontContext();
}


void HelpItemManager::queueHelpMessage(HelpItem msg)
{
    mQueuedItems.push_back(msg);
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
}


void HelpItemManager::addHelpMessage(HelpItem msg)
{
   // Nothing to do if we are disabled
   if(mDisabled)
      return;

   // Only display a message once
   if(mAlreadySeen[msg])
      return;

   // Limit the pacing of new items added -- unless item has immediate priority
   if(mFloodControl.getCurrent() > 0 && helpItems[msg].priority != Immediate)
      return;

   mHelpItems.push_back(msg);
   mHelpTimer.push_back(Timer(10000));    // Display time
   mHelpFading.push_back(false);

   mAlreadySeen[msg] = true;

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


void HelpItemManager::enable()
{
   mDisabled = false;
}


void HelpItemManager::disable()
{
   mDisabled = true;
}



} } // Nested namespace
