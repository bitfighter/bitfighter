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

// Destructor
HelpItemManager::~HelpItemManager()
{
   // Do nothing
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

      addInlineHelpItem(queuedMessage);
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


static const S32 FontSize = 18;
static const S32 FontGap  = 6;

// DON'T PANIC!!! THIS IS A ROUGH INTERMEDIATE FORM THAT WILL BE SIMPLIFIED AND STREAMLINED!!!

void getSymbolShape(const InputCodeManager *inputCodeManager, const string &symbolName, Vector<SymbolShapePtr> &symbols)
{
   // The following will return KEY_UNKNOWN if symbolName is not recognized as a known binding
   InputCode inputCode = inputCodeManager->getKeyBoundToBindingCodeName(symbolName);
   
   if(inputCode != KEY_UNKNOWN)
      symbols.push_back(SymbolString::getControlSymbol(inputCode));

   else if(symbolName == "LOADOUT_ICON")
      symbols.push_back(SymbolString::getSymbolGear());
   
   else if(symbolName == "CHANGEWEP")
   {
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP1)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP2)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP3)));
   }

   else if(symbolName == "MOVEMENT")
   {
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_UP)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_DOWN)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_LEFT)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_RIGHT)));
   }

   else if(symbolName == "MODULE_CTRLS") {}

   else 
      symbols.push_back(SymbolShapePtr(new SymbolText("Unknown Symbol: " + symbolName, FontSize, HelpItemContext)));
}


static void symbolParse(const InputCodeManager *inputCodeManager, string &str, Vector<SymbolShapePtr > &symbols)
{
   std::size_t offset = 0;

   while(true)
   {
      std::size_t startPos = str.find("[[", offset);      // If this isn't here, no further searching is necessary
      std::size_t endPos   = str.find("]]", offset + 2);

      if(startPos == string::npos || endPos == string::npos)
      {
         // No further symbols herein, convert the rest to text symbol and exit
         symbols.push_back(SymbolShapePtr(new SymbolText(str.substr(offset), FontSize, HelpItemContext)));
         return;
      }

      symbols.push_back(SymbolShapePtr(new SymbolText(str.substr(offset, startPos - offset), FontSize, HelpItemContext)));

      getSymbolShape(inputCodeManager, str.substr(startPos + 2, endPos - startPos - 2), symbols);    // + 2 to advance past the "[["

      offset = endPos + 2;
   }
}


static S32 doRenderMessages(const InputCodeManager *inputCodeManager, const char **messages, S32 yPos)
{
   // Final item in messages array will be NULL; loop until we hit that
   for(S32 i = 0; messages[i]; i++)
   {
      TNLAssert(i < MAX_LINES, "Too many lines... better increase MAX_LINES!");

      // Do some token subsititution for dynamic elements such as keybindings
      string renderStr(messages[i]);
      
      Vector<SymbolShapePtr> symbols;
      symbolParse(inputCodeManager, renderStr, symbols);

      UI::SymbolString symbolString(symbols, FontSize, HUDContext);
      symbolString.render(400, yPos, AlignmentCenter);

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


void HelpItemManager::addInlineHelpItem(HelpItem msg)
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
