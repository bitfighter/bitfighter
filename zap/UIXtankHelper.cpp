//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIXtankHelper.h"

#include "UIGame.h"
#include "UIManager.h"
#include "UIInstructions.h"
#include "Colors.h"
#include "DisplayManager.h"
#include "ClientGame.h"
#include "FontManager.h"
#include "gameObjectRender.h"
#include "RenderUtils.h"
#include "Renderer.h"
#include "ship.h"

#include "stringUtils.h"

#include <cmath>

#ifdef TNL_OS_WIN32
#  include <windows.h>     // For ARRAYSIZE
#endif

namespace Zap
{

// For clarity and consistency with other helpers
#define UNSEL_COLOR &Colors::overlayMenuUnselectedItemColor

static const S32 kXtankMaxWeaponSlots = 4;


// Keys used for the 14 bodies: 1-9, 0, A-D.
static const InputCode sBodyKeys[XtankBodyCount] =
{
   KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7,
   KEY_8, KEY_9, KEY_0, KEY_A, KEY_B, KEY_C, KEY_D,
};

// Dynamic key assignment: returns the InputCode for index i (0-based).
// Uses 1-9 for indices 0-8, then A-Z for indices 9-34.
// Covers up to 35 items total (9 numbers + 26 letters).
static InputCode getKeyForIndex(S32 index)
{
   if(index >= 0 && index <= 8)
      return (InputCode)(KEY_1 + index);  // 1-9
   else if(index >= 9 && index <= 34)
      return (InputCode)(KEY_A + (index - 9));  // A-Z
   return KEY_UNKNOWN;
}

// Keys used for heat-sink count (6 options, keys 1-6).
static const InputCode sHeatSinkKeys[XtankHeatSinkMax] =
{
   KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
};


////////////////////////////////////////
////////////////////////////////////////

// Constructor
UIXtankHelper::UIXtankHelper()
{
   mPhase      = 0;
   mSlotCount  = 0;
   mHighlightedIndex = 0;
   mBodyButtonsWidth        = 0;
   mBodyItemsDisplayWidth   = 0;
   mEngineButtonsWidth      = 0;
   mEngineItemsDisplayWidth = 0;
   mTreadButtonsWidth       = 0;
   mTreadItemsDisplayWidth  = 0;
   mHeatSinkButtonsWidth      = 0;
   mHeatSinkItemsDisplayWidth = 0;
   mWeaponButtonsWidth      = 0;
   mWeaponItemsDisplayWidth = 0;
}


// Destructor
UIXtankHelper::~UIXtankHelper()
{
   // Do nothing
}


HelperMenu::HelperMenuType UIXtankHelper::getType() { return XtankHelperType; }


// Build the body-selection menu items.
void UIXtankHelper::buildBodyItems()
{
   mBodyItems.clear();
   for(S32 i = 0; i < XtankBodyCount; i++)
   {
      const TankPhysicsInfo &physics = xtankPhysicsInfos[i];
      S32 turrets = xtankTurretInfos[i].count;

      // Speed class string (rough bands)
      const char *spdClass = (physics.maxSpeed >= 550) ? "Fast" :
                             (physics.maxSpeed >= 480) ? "Med"  : "Slow";

      // Armor class string (armor < 0.8 = heavy, < 1.0 = medium, else light)
      const char *armClass = (physics.armor <= 0.7f) ? "Heavy" :
                             (physics.armor <= 0.95f) ? "Med"   : "Light";

      // We store per-item help text in persistent strings (one per body).
      // Use static storage so the c_str() pointers remain valid.
      static string helpTexts[XtankBodyCount];
      helpTexts[i] = string("SPD:") + spdClass +
                     "  ARM:" + armClass +
                     "  TURR:" + itos(turrets);

      OverlayMenuItem item;
      item.key                 = sBodyKeys[i];
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)i;
      item.name                = xtankBodyNames[i];
      item.itemColor           = UNSEL_COLOR;
      item.help                = helpTexts[i].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;

      mBodyItems.push_back(item);
   }
}


// Build the engine-selection menu items.
void UIXtankHelper::buildEngineItems()
{
   mEngineItems.clear();

   for(S32 e = 0; e < XtankEngineCount; e++)
   {
      const XtankEngineInfo &info = xtankEngineInfos[e];

      // Generate help text from engine stats
      static string engineHelpTexts[XtankEngineCount];
      char buf[128];
      dSprintf(buf, sizeof(buf), "Spd:%.0f%% Acc:%.0f%% Pwr:%d Wt:%d",
               info.speedMult * 100.0f, info.accelMult * 100.0f,
               info.power, info.weight);
      engineHelpTexts[e] = buf;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(e);
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)e;
      item.name                = info.name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = engineHelpTexts[e].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mEngineItems.push_back(item);
   }
}


// Build the tread-selection menu items.
void UIXtankHelper::buildTreadItems()
{
   mTreadItems.clear();

   for(S32 t = 0; t < XtankTreadCount; t++)
   {
      const XtankTreadInfo &info = xtankTreadInfos[t];

      // Generate help text from tread stats
      static string treadHelpTexts[XtankTreadCount];
      char buf[128];
      dSprintf(buf, sizeof(buf), "Friction:%.2f", info.friction);
      treadHelpTexts[t] = buf;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(t);
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)t;
      item.name                = info.name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = treadHelpTexts[t].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mTreadItems.push_back(item);
   }
}


// Build the heat-sink count selection menu items (1-6).
void UIXtankHelper::buildHeatSinkItems()
{
   mHeatSinkItems.clear();
   static string hsHelpTexts[XtankHeatSinkMax];  // persistent storage

   for(S32 n = XtankHeatSinkMin; n <= XtankHeatSinkMax; n++)
   {
      S32 idx = n - XtankHeatSinkMin;
      F32 mult = xtankHeatSinkFireDelayMult(n);
      S32 pct  = (S32)((1.0f - mult) * 100.0f + 0.5f);

      if(pct == 0)
         hsHelpTexts[idx] = "No fire-rate bonus";
      else
      {
         hsHelpTexts[idx] = itos(pct) + "% faster weapon cycling";
      }

      static char namesBuf[XtankHeatSinkMax][16];
      dSprintf(namesBuf[idx], sizeof(namesBuf[idx]), "Heat Sinks: %d", n);

      OverlayMenuItem item;
      item.key                 = sHeatSinkKeys[idx];
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)n;
      item.name                = namesBuf[idx];
      item.itemColor           = UNSEL_COLOR;
      item.help                = hsHelpTexts[idx].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mHeatSinkItems.push_back(item);
   }
}


// Build the weapon-selection menu items for a given slot.
void UIXtankHelper::buildWeaponItems()
{
   mWeaponItems.clear();

   // First option is always "None" (key 0).
   {
      OverlayMenuItem item;
      item.key                 = KEY_0;
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)XtankWeaponNone + 1;  // offset so 0 = None stored safely
      item.name                = "None";
      item.itemColor           = UNSEL_COLOR;
      item.help                = "Empty slot";
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponItems.push_back(item);
   }

   // One entry per weapon (dynamically assigned keys 1-9, A-P).
   for(S32 w = 0; w < XtankWeaponCount; w++)
   {
      OverlayMenuItem item;
      item.key                 = getKeyForIndex(w);  // Use dynamic key assignment
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)w;
      item.name                = xtankWeaponInfos[w].name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = "";
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponItems.push_back(item);
   }
}


// Update the colors of items in a menu to highlight the selected one.
// The highlighted item (at mHighlightedIndex) gets the selected color,
// all others get the unselected color.
void UIXtankHelper::updateItemColors(Vector<OverlayMenuItem> &items)
{
   for(S32 i = 0; i < items.size(); i++)
   {
      if(i == mHighlightedIndex)
      {
         items[i].itemColor = &Colors::overlayMenuSelectedItemColor;
         items[i].helpColor = &Colors::overlayMenuSelectedItemColor;
      }
      else
      {
         items[i].itemColor = UNSEL_COLOR;
         items[i].helpColor = UNSEL_COLOR;
      }
   }
}


void UIXtankHelper::onActivated()
{
   mPhase = 0;
   mHighlightedIndex = 0;

   // Pre-populate the design from the ship's current state.
   Ship *ship = getGame()->getLocalPlayerShip();
   if(ship)
   {
      mDesignInProgress = ship->mXtankDesign;
      if(mDesignInProgress.bodyIndex < 0)
         mDesignInProgress.initForBody(0);   // Default to first body if none active
   }
   else
   {
      mDesignInProgress.initForBody(0);
   }

   // Derive slot count from the current body.
   {
      S32 bodyIdx = (S32)mDesignInProgress.bodyIndex;
      if(bodyIdx < 0 || bodyIdx >= XtankBodyCount) bodyIdx = 0;
      mSlotCount = xtankTurretInfos[bodyIdx].count;
   }

   buildBodyItems();
   buildEngineItems();
   buildTreadItems();
   buildHeatSinkItems();
   buildWeaponItems();

   mBodyButtonsWidth      = getButtonWidth(mBodyItems.address(), mBodyItems.size());
   mBodyItemsDisplayWidth = getMaxItemWidth(mBodyItems.address(), mBodyItems.size());

   mEngineButtonsWidth      = getButtonWidth(mEngineItems.address(), mEngineItems.size());
   mEngineItemsDisplayWidth = getMaxItemWidth(mEngineItems.address(), mEngineItems.size());

   mTreadButtonsWidth      = getButtonWidth(mTreadItems.address(), mTreadItems.size());
   mTreadItemsDisplayWidth = getMaxItemWidth(mTreadItems.address(), mTreadItems.size());

   mHeatSinkButtonsWidth      = getButtonWidth(mHeatSinkItems.address(), mHeatSinkItems.size());
   mHeatSinkItemsDisplayWidth = getMaxItemWidth(mHeatSinkItems.address(), mHeatSinkItems.size());

   mWeaponButtonsWidth      = getButtonWidth(mWeaponItems.address(), mWeaponItems.size());
   mWeaponItemsDisplayWidth = getMaxItemWidth(mWeaponItems.address(), mWeaponItems.size());

   setExpectedWidth(getTotalDisplayWidth(mBodyButtonsWidth, mBodyItemsDisplayWidth));

   // Pre-select the highlighted item to match the existing design.
   setHighlightedIndexForPhase(PHASE_BODY);

   Parent::onActivated();
}


void UIXtankHelper::render()
{
   if(mPhase == PHASE_BODY)          updateItemColors(mBodyItems);
   else if(mPhase == PHASE_ENGINE)   updateItemColors(mEngineItems);
   else if(mPhase == PHASE_TREADS)   updateItemColors(mTreadItems);
   else if(mPhase == PHASE_HEATSINK) updateItemColors(mHeatSinkItems);
   else                              updateItemColors(mWeaponItems);

   renderFloatingMenus();
   renderPreviewPanel();
}


// Return true if the key was handled.
bool UIXtankHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))   // Check for cancel keys
      return true;

   // LEFT arrow / BACKSPACE: carousel backward navigation.
   if(inputCode == KEY_LEFT || inputCode == KEY_BACKSPACE)
   {
      navigateBackward();
      return true;
   }

   // RIGHT arrow: carousel forward navigation.
   if(inputCode == KEY_RIGHT)
   {
      navigateForward();
      return true;
   }

   // UP/DOWN arrows cycle through the highlighted item in the current phase.
   S32 itemCount = currentPhaseItemCount();
   if(itemCount > 0)
   {
      if(inputCode == KEY_UP)
      {
         mHighlightedIndex = (mHighlightedIndex - 1 + itemCount) % itemCount;
         return true;
      }
      if(inputCode == KEY_DOWN)
      {
         mHighlightedIndex = (mHighlightedIndex + 1) % itemCount;
         return true;
      }
      // ENTER key selects the currently highlighted item
      if(inputCode == KEY_ENTER)
      {
         // Simulate pressing the key for the highlighted item
         if(mPhase == PHASE_BODY && mHighlightedIndex < mBodyItems.size())
            return processInputCode(mBodyItems[mHighlightedIndex].key);
         else if(mPhase == PHASE_ENGINE && mHighlightedIndex < mEngineItems.size())
            return processInputCode(mEngineItems[mHighlightedIndex].key);
         else if(mPhase == PHASE_TREADS && mHighlightedIndex < mTreadItems.size())
            return processInputCode(mTreadItems[mHighlightedIndex].key);
         else if(mPhase == PHASE_HEATSINK && mHighlightedIndex < mHeatSinkItems.size())
            return processInputCode(mHeatSinkItems[mHighlightedIndex].key);
         else if(mPhase >= PHASE_WEAPONS && mHighlightedIndex < mWeaponItems.size())
            return processInputCode(mWeaponItems[mHighlightedIndex].key);
      }
   }

   if(mPhase == PHASE_BODY)
   {
      // --- Body selection ---
      for(S32 i = 0; i < mBodyItems.size(); i++)
      {
         if(inputCode == mBodyItems[i].key)
         {
            S32 bodyIdx = (S32)mBodyItems[i].itemIndex;
            mDesignInProgress.initForBody(bodyIdx);
            mSlotCount = xtankTurretInfos[bodyIdx].count;
            mPhase = PHASE_ENGINE;

            // Expand the menu width to fit the engine list.
            S32 newWidth = getTotalDisplayWidth(mEngineButtonsWidth, mEngineItemsDisplayWidth);
            setExpectedWidth_MidTransition(newWidth);
            resetScrollTimer();
            setHighlightedIndexForPhase(PHASE_ENGINE);
            return true;
         }
      }
   }
   else if(mPhase == PHASE_ENGINE)
   {
      // --- Engine selection ---
      for(S32 e = 0; e < mEngineItems.size(); e++)
      {
         if(inputCode == mEngineItems[e].key)
         {
            mDesignInProgress.engineType = (XtankEngine)mEngineItems[e].itemIndex;
            mPhase = PHASE_TREADS;

            S32 newWidth = getTotalDisplayWidth(mTreadButtonsWidth, mTreadItemsDisplayWidth);
            setExpectedWidth_MidTransition(newWidth);
            resetScrollTimer();
            setHighlightedIndexForPhase(PHASE_TREADS);
            return true;
         }
      }
   }
   else if(mPhase == PHASE_TREADS)
   {
      // --- Tread selection ---
      for(S32 t = 0; t < mTreadItems.size(); t++)
      {
         if(inputCode == mTreadItems[t].key)
         {
            mDesignInProgress.treadType = (XtankTread)mTreadItems[t].itemIndex;
            mPhase = PHASE_HEATSINK;

            S32 newWidth = getTotalDisplayWidth(mHeatSinkButtonsWidth, mHeatSinkItemsDisplayWidth);
            setExpectedWidth_MidTransition(newWidth);
            resetScrollTimer();
            setHighlightedIndexForPhase(PHASE_HEATSINK);
            return true;
         }
      }
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      // --- Heat sink count selection ---
      for(S32 n = 0; n < (XtankHeatSinkMax - XtankHeatSinkMin + 1); n++)
      {
         if(inputCode == sHeatSinkKeys[n])
         {
            mDesignInProgress.heatSinkCount = (S8)(n + XtankHeatSinkMin);
            advanceToNextPhaseOrFinish();
            return true;
         }
      }
   }
   else
   {
      // --- Weapon slot selection ---
      S32 slot = mPhase - PHASE_WEAPONS;

      // Check all weapon items (including None at index 0)
      for(S32 i = 0; i < mWeaponItems.size(); i++)
      {
         if(inputCode == mWeaponItems[i].key)
         {
            // First item is "None", remaining items are weapons
            if(i == 0)
               mDesignInProgress.weapons[slot] = XtankWeaponNone;
            else
               mDesignInProgress.weapons[slot] = (XtankWeapon)mWeaponItems[i].itemIndex;

            advanceToNextPhaseOrFinish();
            return true;
         }
      }
   }

   return false;
}


void UIXtankHelper::advanceToNextPhaseOrFinish()
{
   mPhase++;
   if(mPhase >= PHASE_WEAPONS + mSlotCount)
   {
      applyDesign();
   }
   else if(mPhase == PHASE_WEAPONS)
   {
      // Transitioning into first weapon slot
      S32 newWidth = getTotalDisplayWidth(mWeaponButtonsWidth, mWeaponItemsDisplayWidth);
      setExpectedWidth_MidTransition(newWidth);
      resetScrollTimer();
      setHighlightedIndexForPhase(mPhase);
   }
   else
   {
      resetScrollTimer();
      setHighlightedIndexForPhase(mPhase);
   }
}


void UIXtankHelper::navigateForward()
{
   const S32 totalPhases = PHASE_WEAPONS + (mSlotCount > 0 ? mSlotCount : 4);
   if(totalPhases < 1)
      return;

   mPhase = (mPhase + 1) % totalPhases;
   S32 newWidth = widthForPhase(mPhase);
   setExpectedWidth_MidTransition(newWidth);
   resetScrollTimer();
   setHighlightedIndexForPhase(mPhase);
}


// Move to the previous phase, restoring the highlighted index from the
// existing design so the player sees their previous choice pre-selected.
void UIXtankHelper::navigateBackward()
{
   const S32 totalPhases = PHASE_WEAPONS + (mSlotCount > 0 ? mSlotCount : 4);

   mPhase = (mPhase - 1 + totalPhases) % totalPhases;
   S32 newWidth = widthForPhase(mPhase);
   setExpectedWidth_MidTransition(newWidth);
   resetScrollTimer();
   setHighlightedIndexForPhase(mPhase);
}


// Returns the display width for the helper panel at the given phase.
S32 UIXtankHelper::widthForPhase(S32 phase) const
{
   if(phase == PHASE_BODY)     return getTotalDisplayWidth(mBodyButtonsWidth,     mBodyItemsDisplayWidth);
   if(phase == PHASE_ENGINE)   return getTotalDisplayWidth(mEngineButtonsWidth,   mEngineItemsDisplayWidth);
   if(phase == PHASE_TREADS)   return getTotalDisplayWidth(mTreadButtonsWidth,    mTreadItemsDisplayWidth);
   if(phase == PHASE_HEATSINK) return getTotalDisplayWidth(mHeatSinkButtonsWidth, mHeatSinkItemsDisplayWidth);
   return getTotalDisplayWidth(mWeaponButtonsWidth, mWeaponItemsDisplayWidth);  // PHASE_WEAPONS+
}


// Set mHighlightedIndex to reflect the current value in mDesignInProgress for
// the given phase, so when the player navigates back they see their own choice
// already highlighted.
void UIXtankHelper::setHighlightedIndexForPhase(S32 phase)
{
   if(phase == PHASE_BODY)
   {
      mHighlightedIndex = (mDesignInProgress.bodyIndex >= 0) ? (S32)mDesignInProgress.bodyIndex : 0;
   }
   else if(phase == PHASE_ENGINE)
   {
      mHighlightedIndex = (S32)mDesignInProgress.engineType;
   }
   else if(phase == PHASE_TREADS)
   {
      mHighlightedIndex = (S32)mDesignInProgress.treadType;
   }
   else if(phase == PHASE_HEATSINK)
   {
      mHighlightedIndex = (S32)mDesignInProgress.heatSinkCount - XtankHeatSinkMin;
   }
   else
   {
      // Weapon slot: find the matching entry in mWeaponItems.
      // Slot index must be within the weapons[] array bounds (max 4 slots).
      S32 slot = phase - PHASE_WEAPONS;
      if(slot < 0 || slot >= (S32)(sizeof(mDesignInProgress.weapons) / sizeof(mDesignInProgress.weapons[0])))
      {
         mHighlightedIndex = 0;
         return;
      }
      XtankWeapon w = mDesignInProgress.weapons[slot];
      if(w == XtankWeaponNone)
      {
         mHighlightedIndex = 0;  // "None" is the first item
         return;
      }
      // Items 1+ map to weapon index = itemIndex.
      mHighlightedIndex = 0;
      for(S32 i = 1; i < mWeaponItems.size(); i++)
      {
         if((S32)mWeaponItems[i].itemIndex == (S32)w)
         {
            mHighlightedIndex = i;
            break;
         }
      }
   }
}


const Vector<OverlayMenuItem> *UIXtankHelper::getItemsForPhase(S32 phase) const
{
   if(phase == PHASE_BODY)     return &mBodyItems;
   if(phase == PHASE_ENGINE)   return &mEngineItems;
   if(phase == PHASE_TREADS)   return &mTreadItems;
   if(phase == PHASE_HEATSINK) return &mHeatSinkItems;
   return &mWeaponItems;
}


std::string UIXtankHelper::getSelectedLabelForPhase(S32 phase) const
{
   if(phase == PHASE_BODY)
   {
      S32 idx = mDesignInProgress.bodyIndex;
      if(idx < 0 || idx >= XtankBodyCount) idx = 0;
      return xtankBodyNames[idx];
   }

   if(phase == PHASE_ENGINE)
   {
      S32 idx = (S32)mDesignInProgress.engineType;
      if(idx < 0 || idx >= XtankEngineCount) idx = 0;
      return xtankEngineInfos[idx].name;
   }

   if(phase == PHASE_TREADS)
   {
      S32 idx = (S32)mDesignInProgress.treadType;
      if(idx < 0 || idx >= XtankTreadCount) idx = 0;
      return xtankTreadInfos[idx].name;
   }

   if(phase == PHASE_HEATSINK)
   {
      S32 n = (S32)mDesignInProgress.heatSinkCount;
      if(n < XtankHeatSinkMin) n = XtankHeatSinkMin;
      if(n > XtankHeatSinkMax) n = XtankHeatSinkMax;
      return std::string("Heat Sinks: ") + itos(n);
   }

   S32 slot = phase - PHASE_WEAPONS;
   if(slot < 0 || slot >= kXtankMaxWeaponSlots || slot >= mSlotCount)
      return "";

   XtankWeapon w = mDesignInProgress.weapons[slot];
   if(w == XtankWeaponNone)
      return "None";
   if((S32)w < 0 || (S32)w >= XtankWeaponCount)
      return "None";

   return xtankWeaponInfos[(S32)w].name;
}


void UIXtankHelper::renderInlineHighlightedDetails(S32 left, S32 right, S32 yTop) const
{
   Renderer &r = Renderer::get();
   static const S32 STAT_SZ = 12;
   static const S32 GAP = STAT_SZ + 4;
   S32 y = yTop;

   r.setColor(Colors::gray45);
   drawHorizLine(left, right, y - 6);

   r.setColor(Colors::gray70);
   drawString(left, y, STAT_SZ, "Highlighted item");
   y += GAP;

   r.setColor(Colors::cyan);
   if(mPhase == PHASE_BODY)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankBodyCount) idx = 0;
      const TankPhysicsInfo &phy = xtankPhysicsInfos[idx];
      drawStringf(left, y, STAT_SZ, "Speed %d  Rev %d", (S32)phy.maxSpeed, (S32)phy.maxReverseSpeed);
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Accel %d  Turn %.1f", (S32)phy.acceleration, phy.turnRate);
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Armor %.2f  Turrets %d", phy.armor, xtankTurretInfos[idx].count);
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight %d  Cost %d", body_stat[idx].weight, body_stat[idx].cost);
   }
   else if(mPhase == PHASE_ENGINE)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankEngineCount) idx = 0;
      const XtankEngineInfo &ei = xtankEngineInfos[idx];
      drawStringf(left, y, STAT_SZ, "Top speed %+d%%", S32((ei.speedMult - 1.0f) * 100.0f + 0.5f));
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Acceleration %+d%%", S32((ei.accelMult - 1.0f) * 100.0f + 0.5f));
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Power %d  Fuel %d", ei.power, ei.fuel);
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight %d  Cost %d", ei.weight, ei.cost);
   }
   else if(mPhase == PHASE_TREADS)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankTreadCount) idx = 0;
      const XtankTreadInfo &ti = xtankTreadInfos[idx];
      drawStringf(left, y, STAT_SZ, "Turn/handling mult %.2f", ti.friction);
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost %d", ti.cost);
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      S32 n = mHighlightedIndex + XtankHeatSinkMin;
      if(n < XtankHeatSinkMin) n = XtankHeatSinkMin;
      if(n > XtankHeatSinkMax) n = XtankHeatSinkMax;
      drawStringf(left, y, STAT_SZ, "Count %d  Fire-rate +%d%%",
                  n, S32((1.0f - xtankHeatSinkFireDelayMult(n)) * 100.0f + 0.5f));
      y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight %d  Cost %d",
                  heatSinkStat.weight * n, heatSinkStat.cost * n);
   }
   else
   {
      S32 idx = mHighlightedIndex;
      if(idx <= 0)
      {
         drawString(left, y, STAT_SZ, "Empty slot");
      }
      else
      {
         S32 w = idx - 1;
         if(w < 0 || w >= XtankWeaponCount) w = 0;
         const XtankWeaponInfo &wi = xtankWeaponInfos[w];
         drawStringf(left, y, STAT_SZ, "Delay %dms  Energy %d", (S32)xtankFireDelayMs(wi), (S32)wi.heat);
         y += GAP;
         drawStringf(left, y, STAT_SZ, "Proj %du/s  Life %dms", (S32)xtankProjVelocity(wi), (S32)xtankProjLiveTime(wi));
         y += GAP;
         drawStringf(left, y, STAT_SZ, "Weight %d  Cost %d", wi.weight, wi.cost);
      }
   }
}


void UIXtankHelper::renderFloatingMenus()
{
   const Vector<OverlayMenuItem> *items = getItemsForPhase(mPhase);
   if(!items || items->size() <= 0)
      return;

   static const S32 CENTER_LEFT  = 120;
   static const S32 CENTER_RIGHT = 630;
   static const S32 CENTER_TOP   = 125;
   static const S32 CENTER_BOT   = 500;
   static const S32 ADJ_TOP      = 425;
   static const S32 ADJ_BOT      = 548;
   static const S32 LEFT_ADJ_L   = 26;
   static const S32 LEFT_ADJ_R   = 235;
   static const S32 RIGHT_ADJ_L  = 515;
   static const S32 RIGHT_ADJ_R  = 724;
   static const S32 CORNER       = 8;

   auto phaseTitle = [this](S32 phase) -> std::string
   {
      if(phase == PHASE_BODY)     return "Body";
      if(phase == PHASE_ENGINE)   return "Engine";
      if(phase == PHASE_TREADS)   return "Treads";
      if(phase == PHASE_HEATSINK) return "Heat Sinks";
      S32 slot = phase - PHASE_WEAPONS;
      return std::string("Weapon ") + itos(slot + 1);
   };

   Renderer &r = Renderer::get();
   FontManager::pushFontContext(HelperMenuContext);

   // Active center card
   drawFilledFancyBox(CENTER_LEFT, CENTER_TOP, CENTER_RIGHT, CENTER_BOT, CORNER,
                      Colors::black, 0.78f, Colors::red35);

   const S32 cx = (CENTER_LEFT + CENTER_RIGHT) / 2;
   r.setColor(Colors::white);
   drawCenteredStringf(cx, CENTER_TOP + 10, 18, "%s", phaseTitle(mPhase).c_str());
   r.setColor(Colors::gray40);
   drawHorizLine(CENTER_LEFT + 10, CENTER_RIGHT - 10, CENTER_TOP + 34);

   S32 listY = CENTER_TOP + 48;
   const S32 listGap = 20;
   const S32 visible = 9;
   S32 count = items->size();
   S32 start = mHighlightedIndex - visible / 2;
   if(start < 0) start = 0;
   if(start > MAX(0, count - visible)) start = MAX(0, count - visible);
   S32 end = MIN(count, start + visible);

   for(S32 i = start; i < end; i++)
   {
      if(i == mHighlightedIndex)
      {
         r.setColor(Colors::yellow);
         drawString(CENTER_LEFT + 18, listY, 14, ">");
      }
      r.setColor(i == mHighlightedIndex ? Colors::overlayMenuSelectedItemColor : Colors::overlayMenuUnselectedItemColor);
      drawString(CENTER_LEFT + 34, listY, 14, (*items)[i].name);
      listY += listGap;
   }

   renderInlineHighlightedDetails(CENTER_LEFT + 16, CENTER_RIGHT - 16, CENTER_BOT - 98);

   // Adjacent phase cards (Android-task-style depth cue)
   const S32 totalPhases = PHASE_WEAPONS + (mSlotCount > 0 ? mSlotCount : 4);
   if(totalPhases > 1)
   {
      S32 prevPhase = (mPhase - 1 + totalPhases) % totalPhases;
      S32 nextPhase = (mPhase + 1) % totalPhases;

      drawFilledFancyBox(LEFT_ADJ_L, ADJ_TOP, LEFT_ADJ_R, ADJ_BOT, CORNER,
                         Colors::black, 0.62f, Colors::gray35);
      drawFilledFancyBox(RIGHT_ADJ_L, ADJ_TOP, RIGHT_ADJ_R, ADJ_BOT, CORNER,
                         Colors::black, 0.62f, Colors::gray35);

      r.setColor(Colors::gray80);
      drawCenteredString((LEFT_ADJ_L + LEFT_ADJ_R) / 2, ADJ_TOP + 10, 13, phaseTitle(prevPhase).c_str());
      drawCenteredString((RIGHT_ADJ_L + RIGHT_ADJ_R) / 2, ADJ_TOP + 10, 13, phaseTitle(nextPhase).c_str());

      r.setColor(Colors::gray60);
      drawCenteredString((LEFT_ADJ_L + LEFT_ADJ_R) / 2, ADJ_TOP + 34, 11, getSelectedLabelForPhase(prevPhase).c_str());
      drawCenteredString((RIGHT_ADJ_L + RIGHT_ADJ_R) / 2, ADJ_TOP + 34, 11, getSelectedLabelForPhase(nextPhase).c_str());
   }

   FontManager::popFontContext();
}


void UIXtankHelper::applyDesign()
{
   GameUserInterface *gui = getGame()->getUIManager()->getUI<GameUserInterface>();
   if(gui)
      gui->applyXtankDesign(mDesignInProgress);

   exitHelper();
}


void UIXtankHelper::activateHelp(UIManager *uiManager)
{
   // Direct to the weapons help page.
   uiManager->getUI<InstructionsUserInterface>()->activatePage(
      InstructionsUserInterface::InstructionWeaponProjectiles);
}


// Returns the number of selectable items in the current phase so UP/DOWN
// arrow key cycling knows when to wrap.
S32 UIXtankHelper::currentPhaseItemCount() const
{
   if(mPhase == PHASE_BODY)     return mBodyItems.size();
   if(mPhase == PHASE_ENGINE)   return mEngineItems.size();
   if(mPhase == PHASE_TREADS)   return mTreadItems.size();
   if(mPhase == PHASE_HEATSINK) return mHeatSinkItems.size();
   return mWeaponItems.size();   // weapon phases
}


// Draw overall ship preview/spec panel on the right side of the screen.
// This is intentionally aggregate-focused: highlighted-item details live in
// the active center card, while this panel shows what the ship becomes.
void UIXtankHelper::renderPreviewPanel() const
{
   static const S32 PNL_LEFT   = 680;
   static const S32 PNL_RIGHT  = 1050;
   static const S32 PNL_TOP    = 130;
   static const S32 PNL_BOT    = 585;
   static const S32 PNL_CX     = (PNL_LEFT + PNL_RIGHT) / 2;
   static const S32 CORNER     = 8;

   static const S32 TITLE_SZ   = 16;
   static const S32 STAT_SZ    = 13;
   static const S32 LINE_GAP   = STAT_SZ + 6;
   static const S32 TITLE_Y    = PNL_TOP + 10;
   static const S32 BODY_CY    = PNL_TOP + 118;
   static const S32 STATS_Y    = PNL_TOP + 205;
   static const S32 BUILD_Y    = PNL_TOP + 430;
   static const S32 DOTS_Y     = PNL_TOP + 525;
   static const S32 HINT_Y     = PNL_TOP + 548;
   static const F32 BODY_SCALE = 2.2f;

   drawFilledFancyBox(PNL_LEFT, PNL_TOP, PNL_RIGHT, PNL_BOT, CORNER,
                      Colors::black, 0.75f, Colors::red35);

   FontManager::pushFontContext(HelperMenuContext);
   Renderer &r = Renderer::get();

   XtankDesign preview = mDesignInProgress;
   if(mPhase == PHASE_BODY)
   {
      S32 bodyIdx = mHighlightedIndex;
      if(bodyIdx < 0 || bodyIdx >= XtankBodyCount) bodyIdx = 0;
      XtankDesign bodyPreview;
      bodyPreview.initForBody(bodyIdx);
      bodyPreview.engineType = preview.engineType;
      bodyPreview.treadType = preview.treadType;
      bodyPreview.heatSinkCount = preview.heatSinkCount;
      bodyPreview.armorType = preview.armorType;
      for(S32 i = 0; i < MIN(xtankTurretInfos[bodyIdx].count, kXtankMaxWeaponSlots); i++)
         bodyPreview.weapons[i] = preview.weapons[i];
      preview = bodyPreview;
   }
   else if(mPhase == PHASE_ENGINE)
   {
      preview.engineType = (XtankEngine)mHighlightedIndex;
   }
   else if(mPhase == PHASE_TREADS)
   {
      preview.treadType = (XtankTread)mHighlightedIndex;
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      preview.heatSinkCount = (S8)(mHighlightedIndex + XtankHeatSinkMin);
   }
   else
   {
      S32 slot = mPhase - PHASE_WEAPONS;
      if(slot >= 0 && slot < kXtankMaxWeaponSlots)
      {
         if(mHighlightedIndex <= 0) preview.weapons[slot] = XtankWeaponNone;
         else if(mHighlightedIndex < mWeaponItems.size())
            preview.weapons[slot] = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;
      }
   }

   S32 bodyIdx = preview.bodyIndex;
   if(bodyIdx < 0 || bodyIdx >= XtankBodyCount) bodyIdx = 0;
   S32 engineIdx = (S32)preview.engineType;
   if(engineIdx < 0 || engineIdx >= XtankEngineCount) engineIdx = 0;
   S32 treadIdx = (S32)preview.treadType;
   if(treadIdx < 0 || treadIdx >= XtankTreadCount) treadIdx = 0;
   S32 heatSinks = (S32)preview.heatSinkCount;
   if(heatSinks < XtankHeatSinkMin) heatSinks = XtankHeatSinkMin;
   if(heatSinks > XtankHeatSinkMax) heatSinks = XtankHeatSinkMax;

   const TankPhysicsInfo &phy = xtankPhysicsInfos[bodyIdx];
   const XtankEngineInfo &eng = xtankEngineInfos[engineIdx];
   const XtankTreadInfo  &trd = xtankTreadInfos[treadIdx];

   F32 effSpeed = phy.maxSpeed * eng.speedMult;
   F32 effRev   = phy.maxReverseSpeed * eng.speedMult;
   F32 effAccel = phy.acceleration * eng.accelMult;
   F32 effTurn  = phy.turnRate * trd.friction;
   S32 fireRatePct = S32((1.0f - xtankHeatSinkFireDelayMult(heatSinks)) * 100.0f + 0.5f);

   S32 totalWeight = body_stat[bodyIdx].weight + eng.weight + heatSinkStat.weight * heatSinks;
   S32 totalCost   = body_stat[bodyIdx].cost   + eng.cost   + trd.cost + heatSinkStat.cost * heatSinks;
   S32 slotCount   = xtankTurretInfos[bodyIdx].count;
   for(S32 i = 0; i < slotCount && i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = preview.weapons[i];
      if(w == XtankWeaponNone) continue;
      if((S32)w < 0 || (S32)w >= XtankWeaponCount) continue;
      totalWeight += xtankWeaponInfos[(S32)w].weight;
      totalCost   += xtankWeaponInfos[(S32)w].cost;
   }

   r.setColor(Colors::white);
   drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, "Ship Preview");
   r.setColor(Colors::gray40);
   drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, TITLE_Y + TITLE_SZ + 4);

   static const F32 thrusts[4] = { 0, 0, 0, 0 };
   r.pushMatrix();
   r.translate(F32(PNL_CX), F32(BODY_CY), 0);
   r.scale(BODY_SCALE);
   r.rotate(180.0f, 0, 0, 1.0f);
   renderShip(&xtankBodyInfos[bodyIdx], &Colors::blue, Colors::blue, 1.0f,
              const_cast<F32*>(thrusts), 1.0f, F32(Ship::CollisionRadius), 0,
              false, false, false, false);
   renderXtankTurrets(Point(0, 0), 0, 0, 1.0f,
                      xtankTurretInfos[bodyIdx], &Colors::blue, 1.0f);
   r.popMatrix();

   S32 ty = STATS_Y;
   r.setColor(Colors::cyan);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Body: %s", xtankBodyNames[bodyIdx]); ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Wt: %d   Cost: %d", totalWeight, totalCost); ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Speed: %d   Rev: %d", (S32)effSpeed, (S32)effRev); ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Accel: %d   Turn: %.1f", (S32)effAccel, effTurn); ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Armor: %.2f   Turrets: %d", phy.armor, slotCount); ty += LINE_GAP;
   if(fireRatePct > 0)
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Fire-rate bonus: +%d%%", fireRatePct);
   else
      drawCenteredString(PNL_CX, ty, STAT_SZ, "Fire-rate bonus: base");

   renderFullBuildStats(PNL_CX, BUILD_Y, bodyIdx, engineIdx, treadIdx, heatSinks);
   renderCarouselDots(PNL_CX, DOTS_Y);

   r.setColor(Colors::gray50);
   drawCenteredString(PNL_CX, HINT_Y, 11, "[Up]/[Dn] item  [Lt]/[Rt] carousel");

   FontManager::popFontContext();
}


// Draw a row of small circles indicating the current carousel position.
// Since phase navigation wraps, both arrows are always active.
void UIXtankHelper::renderCarouselDots(S32 cx, S32 y) const
{
   S32 totalPhases = PHASE_WEAPONS + (mSlotCount > 0 ? mSlotCount : 4);

   static const F32 DOT_R      = 4.0f;
   static const F32 DOT_R_SM   = 3.0f;
   static const F32 DOT_STEP   = 14.0f;
   static const S32 ARROW_GAP  = 12;
   static const S32 ARROW_SZ   = 11;

   // Centre the dots row at cx.
   F32 rowWidth   = F32(totalPhases - 1) * DOT_STEP;
   F32 dotsLeft   = F32(cx) - rowWidth * 0.5f;

   Renderer &r = Renderer::get();
   static const S32 SEG = 12;  // polygon segments for each circle

   for(S32 i = 0; i < totalPhases; i++)
   {
      F32 dx  = dotsLeft + F32(i) * DOT_STEP;
      F32 rad = (i == mPhase) ? DOT_R : DOT_R_SM;
      bool filled = (i == mPhase);

      if(i == mPhase)
         r.setColor(Colors::white);
      else if(i < mPhase)
         r.setColor(Colors::gray67);
      else
         r.setColor(Colors::gray40);

      if(filled)
      {
         F32 verts[2 + (SEG + 1) * 2];
         verts[0] = dx;
         verts[1] = F32(y);
         for(S32 s = 0; s <= SEG; s++)
         {
            F32 a = FloatPi * 2.0f * F32(s % SEG) / F32(SEG);
            verts[2 + s * 2]     = dx + rad * cosf(a);
            verts[2 + s * 2 + 1] = F32(y) + rad * sinf(a);
         }
         r.renderVertexArray(verts, 2 + SEG, RenderType::TriangleFan);
      }
      else
      {
         F32 verts[SEG * 2];
         for(S32 s = 0; s < SEG; s++)
         {
            F32 a = FloatPi * 2.0f * F32(s) / F32(SEG);
            verts[s * 2]     = dx + rad * cosf(a);
            verts[s * 2 + 1] = F32(y) + rad * sinf(a);
         }
         r.renderVertexArray(verts, SEG, RenderType::LineLoop);
      }
   }

   // Wrap navigation is always available both directions.
   S32 arrowY = y - ARROW_SZ / 2;
   S32 leftArrowX = S32(dotsLeft) - ARROW_GAP - ARROW_SZ;
   r.setColor(Colors::gray67);
   drawCenteredString(leftArrowX, arrowY, ARROW_SZ, "<");

   S32 rightArrowX = S32(dotsLeft + rowWidth) + ARROW_GAP;
   r.setColor(Colors::gray67);
   drawCenteredString(rightArrowX, arrowY, ARROW_SZ, ">");
}


// Draw a "Combined Build Stats" section showing effective vehicle performance
// with the given (potentially preview) component indices applied.
void UIXtankHelper::renderFullBuildStats(S32 cx, S32 y,
                                         S32 previewBodyIdx, S32 previewEngIdx,
                                         S32 previewTreadIdx, S32 previewHeatSinks) const
{
   static const S32 BSTAT_SZ  = 12;
   static const S32 BSTAT_GAP = BSTAT_SZ + 5;

   // Clamp indices to valid ranges.
   if(previewBodyIdx  < 0 || previewBodyIdx  >= XtankBodyCount)   previewBodyIdx  = 0;
   if(previewEngIdx   < 0 || previewEngIdx   >= XtankEngineCount) previewEngIdx   = 0;
   if(previewTreadIdx < 0 || previewTreadIdx >= XtankTreadCount)  previewTreadIdx = 0;
   if(previewHeatSinks < XtankHeatSinkMin) previewHeatSinks = XtankHeatSinkMin;
   if(previewHeatSinks > XtankHeatSinkMax) previewHeatSinks = XtankHeatSinkMax;

   const TankPhysicsInfo &phy = xtankPhysicsInfos[previewBodyIdx];
   const XtankEngineInfo &eng = xtankEngineInfos[previewEngIdx];
   const XtankTreadInfo  &trd = xtankTreadInfos[previewTreadIdx];

   // Effective combined stats.
   F32 effSpeed = phy.maxSpeed * eng.speedMult;
   F32 effAccel = phy.acceleration * eng.accelMult;
   F32 effTurn  = phy.turnRate * trd.friction;
   S32 fireRatePct = S32((1.0f - xtankHeatSinkFireDelayMult(previewHeatSinks)) * 100.0f + 0.5f);

   Renderer &r = Renderer::get();

   // Section divider.
   r.setColor(Colors::gray40);
   S32 divY = y - 4;
   drawHorizLine(cx - 80, cx + 80, divY);

   // Section label.
   r.setColor(Colors::gray50);
   drawCenteredString(cx, divY - BSTAT_SZ - 2, BSTAT_SZ, "CURRENT BUILD");

   // Stat lines.
   S32 ty = y + 4;
   r.setColor(Colors::green);
   drawCenteredStringf(cx, ty, BSTAT_SZ, "Spd: %d  Acc: %d", (S32)effSpeed, (S32)effAccel);
   ty += BSTAT_GAP;
   drawCenteredStringf(cx, ty, BSTAT_SZ, "Turn: %.1f r/s", effTurn);
   ty += BSTAT_GAP;
   if(fireRatePct > 0)
      drawCenteredStringf(cx, ty, BSTAT_SZ, "Fire rate: +%d%%", fireRatePct);
   else
      drawCenteredString(cx, ty, BSTAT_SZ, "Fire rate: base");
}


#undef UNSEL_COLOR

} /* namespace Zap */
