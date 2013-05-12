#include "HelpItemManager.h"

#include "BfObject.h"      // For TypeNumbers
#include "InputCode.h"     // For InputCodeManager
#include "FontManager.h"
#include "SymbolShape.h"
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
HelpItemManager::HelpItemManager(InputCodeManager *inputCodeManager)
{
   mInputCodeManager = inputCodeManager;

   mFloodControl.setPeriod     (10 * 1000);  // Generally, don't show items more frequently than this, in ms
   mPacedTimer.setPeriod       (15 * 1000);  // How often to show a new paced message
   mInitialDelayTimer.setPeriod( 4 * 1000);  // Show nothing until this timer has expired

   mDisabled = false;

#ifdef TNL_DEBUG
   mTestingCtr = -1;
   mTestingTimer.setPeriod(8 * 1000);
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

#ifdef TNL_DEBUG
   mTestingTimer.update(timeDelta);
#endif

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


// DON'T PANIC!!! THIS IS A ROUGH INTERMEDIATE FORM THAT WILL BE SIMPLIFIED AND STREAMLINED!!!

// This method allocates new items (added to shapes vector).  They MUST be cleaned up or we will have leaks!
void getSymbolShape(const InputCodeManager *inputCodeManager, const string &bindingName, Vector<SymbolShape *> &symbols)
{
   if(bindingName == "LOADOUT")
      symbols.push_back(new SymbolGear());

   else if(bindingName == "BINDING_CMDRMAP")
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_CMDRMAP))));

   else if(bindingName == "BINDING_SCRBRD")
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_SCRBRD))));

   else if(bindingName == "BINDING_DROPITEM")
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_DROPITEM))));

   else if(bindingName == "CHANGEWEP")
   {
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP1))));
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP2))));
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP3))));
   }

   else if(bindingName == "MOVEMENT")
   {
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_UP))));
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_DOWN))));
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_LEFT))));
      symbols.push_back(new SymbolText(InputCodeManager::inputCodeToString(inputCodeManager->getBinding(InputCodeManager::BINDING_RIGHT))));
   }

   else 
      symbols.push_back(new SymbolText("Unknown Binding: " + bindingName));
}


static void symbolParse(const InputCodeManager *inputCodeManager, string &str, size_t offset, Vector<SymbolShape *> &symbols);

static void doCheck(const InputCodeManager *inputCodeManager, string &str, const string &what, S32 startPos, Vector<SymbolShape *> &symbols)
{
   size_t len = what.length(); 

   if(str.substr(startPos + 2, len) == what && str.substr(startPos + 2 + len, 2) == "]]")
   {
      getSymbolShape(inputCodeManager, what, symbols);
      symbolParse(inputCodeManager, str, startPos + len + 4, symbols);
      return;
   }
}


static void symbolParse(const InputCodeManager *inputCodeManager, string &str, size_t offset, Vector<SymbolShape *> &symbols)
{
   size_t startPos = str.find("[[", offset);      // If this isn't here, no further searching is necessary
   
   if(startPos == string::npos)
   {
      // No further symbols herein, convert the rest to text symbol and exit
      symbols.push_back(new SymbolText(str.substr(offset)));
      return;
   }

   //InputCodeManager::BINDING_SCRBRD

   symbols.push_back(new SymbolText(str.substr(offset, startPos - offset)));

   doCheck(inputCodeManager, str, "LOADOUT", startPos, symbols);
   doCheck(inputCodeManager, str, "MOVEMENT", startPos, symbols);
   doCheck(inputCodeManager, str, "CHANGEWEP", startPos, symbols);
   doCheck(inputCodeManager, str, "BINDING_DROPITEM", startPos, symbols);
   doCheck(inputCodeManager, str, "BINDING_CMDRMAP", startPos, symbols);
   doCheck(inputCodeManager, str, "BINDING_SCRBRD", startPos, symbols);
}


static S32 doRenderMessages(const InputCodeManager *inputCodeManager, const char **messages, S32 yPos)
{
   static const S32 FontSize = 18;
   static const S32 FontGap  = 6;

   // Final item in messages array will be NULL; loop until we hit that
   for(S32 i = 0; messages[i]; i++)
   {
      TNLAssert(i < MAX_LINES, "Too many lines... better increase MAX_LINES!");

      // Do some token subsititution for dynamic elements such as keybindings
      string renderStr(messages[i]);
      
      //// Temp special handling
      //if(renderStr.find("[[LOADOUT]]") != string::npos)
      //{
      //   Vector<SymbolShape *> symbols;

      //   string s1 = renderStr.substr(0, renderStr.find("[[LOADOUT]]"));
      //   string s2 = renderStr.substr(renderStr.find("[[LOADOUT]]") + 11);

      //   // These will be cleaned up by SymbolString destructor
      //   symbols.push_back(new SymbolText(s1, FontSize, FontContext::HUDContext));
      //   symbols.push_back(new SymbolGear(12));  
      //   symbols.push_back(new SymbolText(s2, FontSize, FontContext::HUDContext));

      //   UI::SymbolString symbolString(symbols, FontSize, FontContext::HUDContext);

      //   symbolString.renderCenter(Point(400, yPos));

      //   return yPos + FontSize + FontGap;
      //}
      
      Vector<SymbolShape *> symbols;
      symbolParse(inputCodeManager, renderStr, 0, symbols);

      UI::SymbolString symbolString(symbols, FontSize, FontContext::HUDContext);
      symbolString.renderCenter(Point(400, yPos));

      yPos += FontSize + FontGap;
   }

   return yPos;
}


void HelpItemManager::renderMessages(S32 yPos) const
{
#  ifdef TNL_DEBUG
      // This bit is for displaying our help messages one-by-one so we can see how they look on-screen
      if(mTestingTimer.getCurrent() > 0)
      {
         FontManager::pushFontContext(HelpItemContext);
         glColor(Colors::red);

         const char **messages = helpItems[mTestingCtr % HelpItemCount].helpMessages;
         doRenderMessages(mInputCodeManager, messages, yPos);

         FontManager::popFontContext();
         return;
      }
#  endif

   if(mInitialDelayTimer.getCurrent() > 0)
      return;
   
   FontManager::pushFontContext(HelpItemContext);

   for(S32 i = 0; i < mHelpItems.size(); i++)
   {
      F32 alpha = mHelpFading[i] ? mHelpTimer[i].getFraction() : 1;
      glColor(Colors::green, alpha);

      const char **messages = helpItems[mHelpItems[i]].helpMessages;
      yPos += doRenderMessages(mInputCodeManager, messages, yPos) + 15;  // Gap between messages
   }

   FontManager::popFontContext();
}


#ifdef TNL_DEBUG
void HelpItemManager::debugShowNextHelpItem()
{
   mTestingCtr++;
   mTestingTimer.reset();
}
#endif


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
