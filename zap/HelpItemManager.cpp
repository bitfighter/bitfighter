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

#include "BfObject.h"            // For TypeNumbers
#include "InputCode.h"           // For InputCodeManager
#include "FontManager.h"
#include "GameSettings.h"

#include "ClientGame.h"
#include "UIGame.h"              // For obtaining loadout indicator width
#include "UIManager.h"
#include "LoadoutIndicator.h"    // For indicator static dimensions
#include "EnergyGaugeRenderer.h"
#include "ScreenInfo.h"          // For canvas width

#include "SymbolShape.h"
#include "Colors.h"
#include "OpenglUtils.h"
#include "RenderUtils.h"
#include "gameObjectRender.h"    // For drawHorizLine
#include "MathUtils.h"           // For min()


using namespace TNL;

namespace Zap {   namespace UI {

static const S32 MAX_LINES = 8;     // Excluding sentinel item

struct HelpItems {
   U8 associatedItem;
   HighlightItem::Whose whose;
   HelpItemManager::Priority priority;
   const char *helpMessages[MAX_LINES + 1];
};


static HelpItems helpItems[] = {
#  define HELP_TABLE_ITEM(a, assItem, whose, priority, items) { assItem, HighlightItem::whose, HelpItemManager::priority, items},
      HELP_ITEM_TABLE
#  undef HELP_TABLE_ITEM
};


// Constructor
HelpItemManager::HelpItemManager(GameSettings *settings)
{
   mGameSettings = settings;
   mInputCodeManager = settings->getInputCodeManager();

   mFloodControl.setPeriod(FloodControlPeriod);       // Generally, don't show items more frequently than this, in ms
   mPacedTimer.setPeriod(PacedTimerPeriod);           // How often to show a new paced message
   mInitialDelayTimer.setPeriod(InitialDelayPeriod);  // Show nothing until this timer has expired

   mGameSettings = settings;

   mEnabled = settings->getShowingInGameHelp();

#ifdef TNL_DEBUG
   mTestingCtr = -1;
   mTestingTimer.setPeriod(8 * 1000);
#endif

   reset();    // Mostly does nothing that is not already done, but good for consistency
   clearAlreadySeenList();
}

// Destructor
HelpItemManager::~HelpItemManager()
{
   // Do nothing
}


// Called when UIGame is activated
void HelpItemManager::reset()
{
   mInitialDelayTimer.reset();   // Provide a short breather before displaying any help items
   mPacedTimer.clear();
   mFloodControl.clear();

   mHighPriorityQueuedItems.clear();
   mLowPriorityQueuedItems.clear();

   mHelpItems.clear();
   mHelpFading.clear();
   mHelpTimer.clear();

#ifdef TNL_DEBUG
   mTestingTimer.clear();
#endif


   mItemsToHighlight.clear();
}


void HelpItemManager::idle(U32 timeDelta, const ClientGame *game)
{
   if(!mEnabled)
      return;

   mInitialDelayTimer.update(timeDelta);

   if(mInitialDelayTimer.getCurrent() > 0)
      return;

   mFloodControl.update(timeDelta);
   mPacedTimer.update(timeDelta);

#ifdef TNL_DEBUG
   mTestingTimer.update(timeDelta);
#endif

   // Check if we can move an item from the queue to the active list -- but don't do it in the final
   // 20 seconds of a game!
   if(mPacedTimer.getCurrent() == 0 && mFloodControl.getCurrent() == 0 && 
            game->getRemainingGameTime() > 20)

      moveItemFromQueueToActiveList(game);
      
   // Expire displayed items
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
            mHelpTimer[i].reset(HelpItemDisplayFadeTime);
         }
      }
}


void HelpItemManager::moveItemFromQueueToActiveList(const ClientGame *game)
{
   TNLAssert(mPacedTimer.getCurrent() == 0 && mFloodControl.getCurrent() == 0, "Expected timers to be clear!");
   S32 itemToShow = 0;

   Vector<WeightedHelpItem> *items = NULL;
   
   bool useHighPriorityQueue = true;

   while(true)
   {
      items = useHighPriorityQueue ? &mHighPriorityQueuedItems : &mLowPriorityQueuedItems;

      if(items->size() <= itemToShow)
      {
         if(useHighPriorityQueue)      // High priority queue exhausted; switch to low priority queue
         {
            itemToShow = 0;
            useHighPriorityQueue = false;
            continue;
         }
         else                          // Low priority queue exhausted; nothing to show... go home
         {
            mPacedTimer.reset();       // Set this just so we don't keep hammering this function all day
            return;
         }
      }
  
      // Handle special case -- want to suppress, but not delete, this item if there are bots in the game
      if(items->get(itemToShow).helpItem == AddBotsItem && game->getBotCount() > 0)
      {
         itemToShow += 1;
         continue;
      }

      break;      // Exit our loop... we have our item list (items) and our itemToShow
   }


   HelpItem queuedMessage = items->get(itemToShow).helpItem;
   items->erase(itemToShow);

   addInlineHelpItem(queuedMessage, true);
   mPacedTimer.reset();
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
      symbols.push_back(SymbolString::getSymbolGear(14));
   
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

   else if(symbolName == "MODULE_CTRL1")
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_MOD1)));

   else if(symbolName == "MODULE_CTRL2")
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_MOD2)));

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


static void renderHelpTextBracket(S32 x, S32 top, S32 bot, S32 stubLen)
{
   drawVertLine (x, top,         bot);    // Vertical bar
   drawHorizLine(x, x + stubLen, top);    // Top stub
   drawHorizLine(x, x + stubLen, bot);    // Bottom stub
}


static void renderIndicatorBracket(S32 left, S32 right, S32 top, S32 stubLen)
{
   drawHorizLine(left,  right, top);   
   drawVertLine (left,  top,   top + stubLen); 
   drawVertLine (right, top,   top + stubLen);
}


// Do some special rendering required by just a couple of items
static void renderMessageDoodads(const ClientGame *game, HelpItem helpItem, S32 textLeft, S32 textTop, S32 textBottom)
{
   // TODO: Remove game->getSettings()->getIniSettings()->showWeaponIndicators option

   textLeft -= 10;      // Provide some buffer between vertical bar and help text

   const S32 stubLen = 15;
   const S32 indicatorHorizontalGap = 5;     // Space between indicator and vertical stubs
   const S32 indicatorVerticalGap = 10;      // Space between indicator and horizontal line

   if(helpItem == ModulesAndWeaponsItem)
   {
      const S32 w = game->getUIManager()->getUI<GameUserInterface>()->getLoadoutIndicatorWidth();
      const S32 x = UI::LoadoutIndicator::LoadoutIndicatorLeftPos;
      const S32 y = UI::LoadoutIndicator::LoadoutIndicatorBottomPos;

      const S32 indicatorTop = y + indicatorVerticalGap;
      const S32 riserBot = (textTop + textBottom) / 2;

      // Some loadouts are long enough that we get a weird display... min fixes that.
      // (This may no longer be a problem now that the help text was shortened.)
      const S32 indicatorMiddle   = min(x + w / 2, textLeft - 15);   

      const S32 indicatorLeft  = x -     indicatorHorizontalGap;                   
      const S32 indicatorRight = x + w + indicatorHorizontalGap;

      renderHelpTextBracket(textLeft, textTop, textBottom, stubLen);
      renderIndicatorBracket(indicatorLeft, indicatorRight, indicatorTop, -stubLen);

      // Lines connecting the two
      drawHorizLine(indicatorMiddle, textLeft,    riserBot);    // Main horizontal
      drawVertLine (indicatorMiddle, indicatorTop, riserBot);    // Main riser
   }

   else if(helpItem == GameTypeAndTimer)
   {
      const Point widthAndHeight = game->getUIManager()->getUI<GameUserInterface>()->getTimeLeftIndicatorWidthAndHeight();
      const S32 w = (S32)widthAndHeight.x;
      const S32 h = (S32)widthAndHeight.y;
      const S32 x = gScreenInfo.getGameCanvasWidth() - UI::TimeLeftRenderer::TimeLeftIndicatorMargin - w;
      const S32 indicatorTop = gScreenInfo.getGameCanvasHeight() - UI::TimeLeftRenderer::TimeLeftIndicatorMargin - h - indicatorVerticalGap;

      const S32 indicatorLeft  = x + w + indicatorHorizontalGap;
      const S32 indicatorRight = x - indicatorHorizontalGap;

      const S32 indicatorMiddle = (indicatorLeft + indicatorRight) / 2;
      const S32 textMiddle = (textTop + textBottom) / 2;

      const S32 textRight = gScreenInfo.getGameCanvasWidth() - textLeft;

      renderHelpTextBracket(textRight, textTop, textBottom, -stubLen);
      renderIndicatorBracket(indicatorLeft, indicatorRight, indicatorTop, stubLen);

      // Lines connecting the two
      drawHorizLine(textRight, indicatorMiddle, textMiddle);
      drawVertLine(indicatorMiddle, textMiddle, indicatorTop);
   }
   else if(helpItem == EnergyGaugeItem)
   {
      const S32 indicatorLeft  = UI::EnergyGaugeRenderer::GaugeLeftMargin - indicatorHorizontalGap;
      const S32 indicatorRight = UI::EnergyGaugeRenderer::GaugeLeftMargin + UI::EnergyGaugeRenderer::GuageWidth + indicatorHorizontalGap;
      const S32 indicatorTop   = gScreenInfo.getGameCanvasHeight() - 
                                          (UI::EnergyGaugeRenderer::GaugeBottomMargin + 
                                           UI::EnergyGaugeRenderer::GaugeHeight + 
                                           UI::EnergyGaugeRenderer::SafetyLineExtend + 
                                           indicatorVerticalGap);

      const S32 textMiddle = (textTop + textBottom) / 2;
      const S32 indicatorMiddle = (indicatorLeft + indicatorRight) / 2;

      renderHelpTextBracket(textLeft, textTop, textBottom, stubLen);
      renderIndicatorBracket(indicatorLeft, indicatorRight, indicatorTop, stubLen);

      // Lines connecting the two
      drawHorizLine(textLeft, indicatorMiddle, textMiddle);
      drawVertLine(indicatorMiddle, textMiddle, indicatorTop);
   }
}


static S32 doRenderMessages(const ClientGame *game, const InputCodeManager *inputCodeManager, HelpItem helpItem, S32 yPos)
{
   const char **messages = helpItems[helpItem].helpMessages;

   S32 lines = 0;
   S32 maxw = 0;
   S32 xPos = gScreenInfo.getGameCanvasWidth() / 2;

   // Final item in messages array will be NULL; loop until we hit that
   for(S32 i = 0; messages[i]; i++)
   {
      TNLAssert(i < MAX_LINES, "Too many lines... better increase MAX_LINES!");

      // Do some token subsititution for dynamic elements such as keybindings
      string renderStr(messages[i]);
      
      Vector<SymbolShapePtr> symbols;
      symbolParse(inputCodeManager, renderStr, symbols);

      UI::SymbolString symbolString(symbols, FontSize, HUDContext);
      symbolString.render(xPos, yPos, AlignmentCenter);

      S32 w = symbolString.getWidth();
      maxw = max(maxw, w);

      yPos += FontSize + FontGap;
      lines++;
   }

   renderMessageDoodads(game, helpItem, xPos - maxw / 2, yPos - (lines + 1) * (FontSize + FontGap), yPos - FontSize + 4);

   return yPos;
}


void HelpItemManager::renderMessages(const ClientGame *game, S32 yPos) const
{
   if(!mEnabled)
      return;

   logprintf("%d",game->getBotCount());  //{P{P

#ifdef TNL_DEBUG
   // This bit is for displaying our help messages one-by-one so we can see how they look on-screen, cycle with CTRL+H
   if(mTestingTimer.getCurrent() > 0)
   {
      FontManager::pushFontContext(HelpItemContext);
      glColor(Colors::red);

      doRenderMessages(game, mInputCodeManager, (HelpItem)(mTestingCtr % HelpItemCount), yPos);

      FontManager::popFontContext();
      return;
   }
#endif

   if(mInitialDelayTimer.getCurrent() > 0)
      return;
   
   FontManager::pushFontContext(HelpItemContext);

   for(S32 i = 0; i < mHelpItems.size(); i++)
   {
      F32 alpha = mHelpFading[i] ? mHelpTimer[i].getFraction() : 1;
      glColor(Colors::green, alpha);

      yPos += doRenderMessages(game, mInputCodeManager, mHelpItems[i], yPos) + 15;  // Gap between messages
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


// Queues up items that are not specific to a particular item or event, such as a tip on how to activate the cmdrs map
// This is not used for things like "This is a soccer ball"
// Now only used internally
void HelpItemManager::queueHelpItem(HelpItem item)
{
   TNLAssert(helpItems[item].priority == PacedHigh || helpItems[item].priority == PacedLow, "This method is only for paced items!");

   // Don't queue items we've already seen
   if(mAlreadySeen[item])
       return;

   WeightedHelpItem weightedItem;
   weightedItem.helpItem = item;
   weightedItem.removalWeight = 0;

   if(helpItems[item].priority == PacedHigh)
      mHighPriorityQueuedItems.push_back(weightedItem);
   else
      mLowPriorityQueuedItems.push_back(weightedItem);
}


// The weight factor allows us to require several events to "vote" for removing an item before 
// it happens... basically once the weights OR to 0xFF, the item is toast.
void HelpItemManager::removeInlineHelpItem(HelpItem item, U8 weight)
{
   TNLAssert(helpItems[item].priority == PacedHigh || helpItems[item].priority == PacedLow, "This method is only for paced items!");

   Vector<WeightedHelpItem> *queue = helpItems[item].priority == PacedHigh ? &mHighPriorityQueuedItems : &mLowPriorityQueuedItems;
   S32 index = -1;
   for(S32 i = 0; i < queue->size(); i++)
      if(queue->get(i).helpItem == item)
      {
         index = i;
         break;
      }

    if(index != -1)
    {
       queue->get(index).removalWeight |= weight;
       if(queue->get(index).removalWeight == 0xFF)
         queue->erase(index);
    }
}


// Clears all message-seen status flags, then writes to the INI
void HelpItemManager::resetInGameHelpMessages()
{
   clearAlreadySeenList();
   saveAlreadySeenList();
}


// Clears all flags; does not save to INI
void HelpItemManager::clearAlreadySeenList()
{
   for(S32 i = 0; i < HelpItemCount; i++)
      mAlreadySeen[i] = false;
}


// Write seen status to INI
void HelpItemManager::saveAlreadySeenList()
{
   mGameSettings->saveHelpItemAlreadySeenList(getAlreadySeenString());
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


// Called whenever some item somewhere thinks it would be a good time to add a help message.
// Items added here are immediately displayed.
void HelpItemManager::addInlineHelpItem(HelpItem item, bool messageCameFromQueue)
{
   // Nothing to do if we are disabled, and only display messages once
   if(!mEnabled || mAlreadySeen[item])
      return;

   // If the item has a priority of paced, we should queue the item rather than display it immediately (unless,
   // of course, it came from the queue!)
   if(!messageCameFromQueue && (helpItems[item].priority == PacedHigh || helpItems[item].priority == PacedLow))
   {
      queueHelpItem(item);
      return;
   }

   // Skip the timer and queued item checks for Now priority items
   if(helpItems[item].priority != Now)
   {
      // Ignore messages while floodControl or initialDelay timers are active
      if((mFloodControl.getCurrent() > 0 || mInitialDelayTimer.getCurrent() > 0))
         return;

      // Don't add if there are high priority queued items waiting
      if(mHighPriorityQueuedItems.size() > 0 && !messageCameFromQueue)  
         return;
   }

   mHelpItems.push_back(item);
   mHelpTimer.push_back(Timer(HelpItemDisplayPeriod));    // Display time
   mHelpFading.push_back(false);

   // Items gets marked as seen after it first flashes on the screen... is this really what we want?
   mAlreadySeen[item] = true;
   saveAlreadySeenList();

   mFloodControl.reset();

   buildItemsToHighlightList();

   return;
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


void HelpItemManager::setEnabled(bool isEnabled)
{
   mEnabled = isEnabled;
}


bool HelpItemManager::isEnabled()
{
   return mEnabled;
}


// Access for testing
const Vector<HelpItem>                          *HelpItemManager::getHelpItemDisplayList() const { return &mHelpItems;               }
const Vector<HelpItemManager::WeightedHelpItem> *HelpItemManager::getHighPriorityQueue()   const { return &mHighPriorityQueuedItems; }
const Vector<HelpItemManager::WeightedHelpItem> *HelpItemManager::getLowPriorityQueue()    const { return &mLowPriorityQueuedItems;  }


HelpItemManager::Priority HelpItemManager::getItemPriority(HelpItem item) const
{
   return helpItems[item].priority;
}





} } // Nested namespace
