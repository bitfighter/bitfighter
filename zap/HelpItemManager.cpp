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
#include "ScissorsManager.h"

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
   U8 associatedObjectTypeNumber;
   bool autoAdd;   
   bool highlightObjectiveArrows;
   HighlightItem::Whose whose;
   HelpItemManager::Priority priority;
   const char *helpMessages[MAX_LINES + 1];
};


static const HelpItems helpItems[] = {
#  define HELP_TABLE_ITEM(a, assItem, autoAdd, highlightObjArrows, whose, priority, items) \
             { assItem, autoAdd, highlightObjArrows, HighlightItem::whose, HelpItemManager::priority, items},
      HELP_ITEM_TABLE
#  undef HELP_TABLE_ITEM
};

// Provide very specific access to above structure (static method)
// Only used for intializing hasHelpItemForObjects[] array in ClientGame
U8 HelpItemManager::getAssociatedObjectType(HelpItem helpItem)
{
   if(helpItems[helpItem].autoAdd)
      return helpItems[helpItem].associatedObjectTypeNumber;
   else
      return UnknownTypeNumber;
}


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

   reset();					// Mostly does nothing that is not already done, but good for consistency
   clearAlreadySeenList();	// Needed?  If not, mostly harmless.
   loadAlreadySeenList();
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


// Display sizes for help items
static const S32 FontSize = 18;
static const S32 FontGap  = 6;
static const S32 InterMsgGap = 15;     // Space btwn adjacent messages


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
   if(mPacedTimer.getCurrent() == 0 && mFloodControl.getCurrent() == 0 && game->getRemainingGameTime() > 20)
      moveItemFromQueueToActiveList(game);
      
   // Expire displayed items
   for(S32 i = 0; i < mHelpTimer.size(); i++)
   {
      if(!mHelpTimer[i].update(timeDelta))
         continue;

      if(mHelpFading[i])   // Rollup period over... kill item
      {
         mHelpItems.erase(i);
         mHelpFading.erase(i);
         mHelpTimer.erase(i);

         buildItemsToHighlightList();
         i--;
      }

      else                 // Display period over... enter rollup mode
      {
         mHelpFading[i] = true;

         // Reset the timer to a new value based on the number of lines in the item -- this
         // will keep the rollup effect going at a constant speed 
         mHelpTimer[i].reset(getRollupPeriod(i));
      }
   }
}


// Time it will take for the displayed item at [index] to have its "roll up" animation play out.
// (Longer items take longer to roll up, given a constant roll up rate.)
S32 HelpItemManager::getRollupPeriod(S32 index) const
{
   TNLAssert(mHelpItems.size() > index, "Index out of range!");
   return (getLinesInHelpItem(index) * (FontSize + FontGap) + InterMsgGap) * 5;    // 5 ms per pixel height
}


S32 HelpItemManager::getLinesInHelpItem(S32 index) const
{
   S32 lines = 0;
   while(helpItems[mHelpItems[index]].helpMessages[lines])
      lines++;

   return lines;
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
      drawHorizLine(indicatorMiddle, textLeft,     riserBot);    // Main horizontal
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


static S32 doRenderMessages(const ClientGame *game, const InputCodeManager *inputCodeManager, HelpItem helpItem, F32 yPos)
{
   const char * const *messages = helpItems[helpItem].helpMessages;

   S32 lines = 0;
   S32 maxw = 0;
   F32 xPos = gScreenInfo.getGameCanvasWidth() / 2.0f;
   S32 yOffset = 0;

   // Final item in messages array will be NULL; loop until we hit that
   for(S32 i = 0; messages[i]; i++)
   {
      TNLAssert(i < MAX_LINES, "Too many lines... better increase MAX_LINES!");

      // Do some token subsititution for dynamic elements such as keybindings
      string renderStr(messages[i]);
      
      Vector<SymbolShapePtr> symbols;
      SymbolString::symbolParse(inputCodeManager, renderStr, symbols, HelpItemContext, FontSize);

      UI::SymbolString symbolString(symbols);
      symbolString.render(xPos, yPos + yOffset, AlignmentCenter);

      S32 w = symbolString.getWidth();
      maxw = max(maxw, w);

      yOffset += FontSize + FontGap;
      lines++;
   }

   S32 leftPos = (S32)xPos - maxw / 2;
   S32 topPos  = (S32)yPos + yOffset - (lines + 1) * (FontSize + FontGap);
   S32 botPos  = (S32)yPos + yOffset - FontSize + 4;    // 4.... just... because?
   renderMessageDoodads(game, helpItem, leftPos, topPos, botPos);

   return yOffset;
}


static ScissorsManager scissorsManager;

void HelpItemManager::renderMessages(const ClientGame *game, F32 yPos, F32 alpha) const
{
#ifdef TNL_DEBUG
   // This bit is for displaying our help messages one-by-one so we can see how they look on-screen, cycle with CTRL+H
   if(mTestingTimer.getCurrent() > 0)
   {
      FontManager::pushFontContext(HelpItemContext);
      glColor(Colors::red, alpha);

      doRenderMessages(game, mInputCodeManager, (HelpItem)(mTestingCtr % HelpItemCount), yPos);

      FontManager::popFontContext();
      return;
   }
#endif

   if(!mEnabled)
      return;
   
   FontManager::pushFontContext(HelpItemContext);

   for(S32 i = 0; i < mHelpItems.size(); i++)      // Iterate over each message being displayed
   {
      glColor(Colors::HelpItemRenderColor, alpha);

      // Height of the message in pixels, including the gap before the next message (even if there isn't one)
      F32 height = F32(getLinesInHelpItem(i) * (FontSize + FontGap)) + InterMsgGap;

      // Offset makes lower items slide up as upper items are rolled up -- when we're not fading, offset
      // is 0; when we are, offset directs doRenderMessages to render with the top of the message higher
      // than normal.  That, combined with scissors clipping, results in the rolling-up effect.
      F32 offset = height * (mHelpFading[i] ? 1 - mHelpTimer[i].getFraction() : 0);

      scissorsManager.enable(mHelpFading[i], game->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode"), 
                             0, yPos - FontSize, gScreenInfo.getGameCanvasWidth(), height);

      doRenderMessages(game, mInputCodeManager, mHelpItems[i], yPos - offset);
      yPos += height - offset;      

      scissorsManager.disable();
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
   TNLAssert(helpItems[item].priority == PacedHigh || helpItems[item].priority == PacedLow || 
             helpItems[item].priority == GameStart, "This method is only for Paced/GameStart items!");

   // Don't queue items we've already seen
   if(mAlreadySeen[item])
       return;

   WeightedHelpItem weightedItem;
   weightedItem.helpItem = item;
   weightedItem.removalWeight = 0;

   if(helpItems[item].priority == PacedHigh || helpItems[item].priority == GameStart)
      mHighPriorityQueuedItems.push_back(weightedItem);
   else
      mLowPriorityQueuedItems.push_back(weightedItem);
}


// The weight factor allows us to require several events to "vote" for removing an item before 
// it happens... basically once the weights OR to 0xFF, the item is toast.
void HelpItemManager::removeInlineHelpItem(HelpItem item, bool markAsSeen, U8 weight)
{
   //TNLAssert(helpItems[item].priority == PacedHigh || helpItems[item].priority == PacedLow, "This method is only for paced items!");
   if(helpItems[item].priority == PacedHigh || helpItems[item].priority == PacedLow)      // for now
   {
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

   if(markAsSeen)
      mAlreadySeen[item] = true;
}


F32 HelpItemManager::getObjectiveArrowHighlightAlpha() const
{
   if(!mEnabled)
      return 0;

   F32 alpha = 0;

   for(S32 i = 0; i < mHelpItems.size(); i++)
      if(helpItems[mHelpItems[i]].highlightObjectiveArrows)
         alpha = max(alpha, mHelpFading[i] ? mHelpTimer[i].getFraction() : 1);

   return alpha;
}


// Clears all message-seen status flags, then writes to the INI
void HelpItemManager::resetInGameHelpMessages()
{
   clearAlreadySeenList();
   saveAlreadySeenList();
}


// Write seen status to INI
void HelpItemManager::saveAlreadySeenList()
{
   mGameSettings->getIniSettings()->mSettings.setVal("HelpItemsAlreadySeenList", getAlreadySeenString());
}


void HelpItemManager::loadAlreadySeenList()
{
   setAlreadySeenString(mGameSettings->getIniSettings()->mSettings.getVal<string>("HelpItemsAlreadySeenList"));
}


const string HelpItemManager::getAlreadySeenString() const
{
   return IniSettings::bitArrayToIniString(mAlreadySeen, HelpItemCount);
}


// Clears all flags; does not save to INI
void HelpItemManager::clearAlreadySeenList()
{
   IniSettings::clearbits(mAlreadySeen, HelpItemCount);
}


// Takes a string; we'll mark a message as being seen every time we encounter a 'Y'
void HelpItemManager::setAlreadySeenString(const string &vals)
{
   IniSettings::iniStringToBitArray(vals, mAlreadySeen, HelpItemCount);

   // Probably should be drawn from a definition elsewhere, but some pairs of messages are dependent on one another.
   // If the first has already been shown, don't show the second
   mAlreadySeen[LoadoutFinishedItem] = mAlreadySeen[LoadoutChangedZoneItem];
}


static inline bool isNeut(S32 objectTeam)                  { return objectTeam == TEAM_NEUTRAL; }
static inline bool isHost(S32 objectTeam)                  { return objectTeam == TEAM_HOSTILE; }
static inline bool isTeam(S32 objectTeam, S32 playerTeam)  { return objectTeam == playerTeam;   }
static inline bool isEnemy(S32 objectTeam, S32 playerTeam) { return objectTeam >= 0 && objectTeam != playerTeam; }

static bool checkWhose(HighlightItem::Whose whose, S32 objectTeam, S32 playerTeam)
{
   switch(whose)
   {
      case HighlightItem::Any:
         return true;

      case HighlightItem::Team:
         return isTeam(objectTeam, playerTeam);

      case HighlightItem::TorNeut:
         return isTeam(objectTeam, playerTeam) || isNeut(objectTeam);

      case HighlightItem::Enemy:
         return isEnemy(objectTeam, playerTeam);

      case HighlightItem::Hostile:
         return isHost(objectTeam);

      case HighlightItem::EorHostile:
         return isEnemy(objectTeam, playerTeam) || isHost(objectTeam);

      case HighlightItem::EorHorN:
         return isEnemy(objectTeam, playerTeam) || isHost(objectTeam) || isNeut(objectTeam);

      case HighlightItem::Neutral:
         return isNeut(objectTeam);

      default:
         TNLAssert(false, "Unknown value of whose!");
         return false;
   }
}


// This signature gets used when the player encounters an object for which we have an associated help item...
void HelpItemManager::addInlineHelpItem(U8 objectType, S32 objectTeam, S32 playerTeam)
{
   // Nothing to do if we are disabled
   if(!mEnabled)
      return;

   // Figure out which help item to show for this object
   for(S32 i = 0; i < HelpItemCount; i++)
      if(helpItems[i].associatedObjectTypeNumber == objectType && checkWhose(helpItems[i].whose, objectTeam, playerTeam))
      {
         addInlineHelpItem(HelpItem(i));
         return;
      }
}


// Called whenever some item somewhere thinks it would be a good time to add a help message.
// Items added here are immediately displayed.
void HelpItemManager::addInlineHelpItem(HelpItem item, bool messageCameFromQueue)
{
   // Nothing to do if we are disabled
   if(!mEnabled)
      return;

   // Only display messages once
   if(mAlreadySeen[item])
      return;

   // If the item has a priority of paced, we should queue the item rather than display it immediately (unless,
   // of course, it came from the queue!)
   if(!messageCameFromQueue)
   {
      if(helpItems[item].priority == GameStart)
         removeGameStartItemsFromQueue();

      Priority pr = helpItems[item].priority;

      // GameStart means remove any GameStart items in queue, and add to high priority queue only if it is empty
      if(pr == PacedHigh || pr == PacedLow || (pr == GameStart && mHighPriorityQueuedItems.size() == 0))
         queueHelpItem(item);

      if(pr == PacedHigh || pr == PacedLow || pr == GameStart)
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


void HelpItemManager::removeGameStartItemsFromQueue()
{
   // Any GameStart items should be at the beginning of the queue, as they are only added if the queue is empty
   while(mHighPriorityQueuedItems.size() > 0 && helpItems[mHighPriorityQueuedItems[0].helpItem].priority == GameStart)
      mHighPriorityQueuedItems.erase(0);
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
      U8 itemType = helpItems[mHelpItems[i]].associatedObjectTypeNumber;

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


bool HelpItemManager::isEnabled() const
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
