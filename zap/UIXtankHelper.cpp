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

namespace Zap
{

// For clarity and consistency with other helpers
#define UNSEL_COLOR &Colors::overlayMenuUnselectedItemColor

static const S32 kXtankMaxWeaponSlots = 4;

// Armor budget = this multiplier × body.size, distributed equally across 4 sides initially.
static const S32 kArmorPointsPerBodySize = 10;


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


// Returns a human-readable side label for a turret based on its mount position
// in body space (+Y = forward/nose).  Thresholds are intentionally generous so
// all layouts produce a useful label.
const char *UIXtankHelper::getTurretSideLabel(S32 bodyIdx, S32 slot)
{
   if(bodyIdx < 0 || bodyIdx >= XtankBodyCount) return "Slot";
   const XtankBodyTurrets &bt = xtankTurretInfos[bodyIdx];
   if(slot < 0 || slot >= bt.count) return "Slot";
   F32 x = bt.turrets[slot].x;
   F32 y = bt.turrets[slot].y;
   if(fabsf(x) < 2.0f && fabsf(y) < 2.0f) return "Center";
   if(fabsf(y) >= fabsf(x))
      return (y > 0.0f) ? "Front" : "Rear";
   return (x > 0.0f) ? "Right" : "Left";
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
UIXtankHelper::UIXtankHelper()
{
   mPhase      = 0;
   mSlotCount  = 0;
   mWeaponSide = 0;
   mHighlightedIndex = 0;
   mTransitionFromPhase = -1;
   mTransitionForward = true;
   mBodyButtonsWidth        = 0;
   mBodyItemsDisplayWidth   = 0;
   mEngineButtonsWidth      = 0;
   mEngineItemsDisplayWidth = 0;
   mTreadButtonsWidth       = 0;
   mTreadItemsDisplayWidth  = 0;
   mArmorButtonsWidth       = 0;
   mArmorItemsDisplayWidth  = 0;
   mArmorSidesButtonsWidth      = 0;
   mArmorSidesItemsDisplayWidth = 0;
   mSuspensionButtonsWidth      = 0;
   mSuspensionItemsDisplayWidth = 0;
   mBumperButtonsWidth      = 0;
   mBumperItemsDisplayWidth = 0;
   mSpecialsButtonsWidth      = 0;
   mSpecialsItemsDisplayWidth = 0;
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


// Build the armor-type selection menu items.
void UIXtankHelper::buildArmorItems()
{
   mArmorItems.clear();
   static string armorHelpTexts[XtankArmorCount];

   for(S32 a = 0; a < XtankArmorCount; a++)
   {
      const XtankArmorInfo &info = xtankArmorInfos[a];
      char buf[128];
      dSprintf(buf, sizeof(buf), "Def:%d Wt:%d/unit Cost:%d/unit", info.defense, info.weight, info.cost);
      armorHelpTexts[a] = buf;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(a);
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)a;
      item.name                = info.name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = armorHelpTexts[a].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mArmorItems.push_back(item);
   }
}


// Build the 4-item armor-sides phase list (Front / Back / Left / Right).
// Items have no hotkeys; point values are edited live with +/- in processInputCode.
void UIXtankHelper::buildArmorSidesItems()
{
   static const char *sideNames[4] = { "Front", "Back", "Left", "Right" };
   mArmorSidesItems.clear();
   for(S32 i = 0; i < 4; i++)
   {
      OverlayMenuItem item;
      item.key                 = KEY_NONE;
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)i;
      item.name                = sideNames[i];
      item.itemColor           = UNSEL_COLOR;
      item.help                = "";
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mArmorSidesItems.push_back(item);
   }
}


// Build the suspension-type selection menu items.
void UIXtankHelper::buildSuspensionItems()
{
   mSuspensionItems.clear();
   static string suspHelpTexts[XtankSuspensionCount];

   for(S32 s = 0; s < XtankSuspensionCount; s++)
   {
      const SuspensionStat &info = suspensionStat[s];
      char buf[128];
      dSprintf(buf, sizeof(buf), "Handling: %+d  Cost: %d", (S32)info.friction, info.cost);
      suspHelpTexts[s] = buf;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(s);
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)s;
      item.name                = info.name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = suspHelpTexts[s].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mSuspensionItems.push_back(item);
   }
}


// Build the bumper-type selection menu items.
void UIXtankHelper::buildBumperItems()
{
   mBumperItems.clear();
   static string bumperHelpTexts[XtankBumperCount];

   for(S32 b = 0; b < XtankBumperCount; b++)
   {
      const BumperStat &info = bumperStat[b];
      char buf[128];
      dSprintf(buf, sizeof(buf), "Elasticity:%.2f  Cost:%d", info.elasticity, info.cost);
      bumperHelpTexts[b] = buf;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(b);
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)b;
      item.name                = info.name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = bumperHelpTexts[b].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mBumperItems.push_back(item);
   }
}


// Build the special equipment selection menu items.
void UIXtankHelper::buildSpecialsItems()
{
   mSpecialsItems.clear();
   static string specHelpTexts[XtankSpecialCount];

   for(S32 s = 0; s < XtankSpecialCount; s++)
   {
      const XtankSpecialInfo &info = xtankSpecialInfos[s];
      char buf[128];
      dSprintf(buf, sizeof(buf), "%s  Wt:%d Sp:%d Cost:%d",
               info.description, info.weight, info.space, info.cost);
      specHelpTexts[s] = buf;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(s);
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)s;
      item.name                = info.name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = specHelpTexts[s].c_str();
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mSpecialsItems.push_back(item);
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
   mWeaponSide = 0;

   // Pre-populate the design from the ship's current state.
   Ship *ship = getGame()->getLocalPlayerShip();
   if(ship && ship->mXtankDesign.bodyIndex >= 0)
   {
      mDesignInProgress = ship->mXtankDesign;
   }
   else
   {
      // No existing design — start from a clean default.
      mDesignInProgress.initForBody(0);
   }

   // Snapshot so ESC can restore the original.
   mOriginalDesign = mDesignInProgress;

   // Derive slot count from the current body.
   {
      S32 bodyIdx = (S32)mDesignInProgress.bodyIndex;
      if(bodyIdx < 0 || bodyIdx >= XtankBodyCount) bodyIdx = 0;
      mSlotCount = xtankTurretInfos[bodyIdx].count;
   }

   buildBodyItems();
   buildEngineItems();
   buildTreadItems();
   buildArmorItems();
   buildArmorSidesItems();
   buildSuspensionItems();
   buildBumperItems();
   buildSpecialsItems();
   buildHeatSinkItems();
   buildWeaponItems();

   mBodyButtonsWidth      = getButtonWidth(mBodyItems.address(), mBodyItems.size());
   mBodyItemsDisplayWidth = getMaxItemWidth(mBodyItems.address(), mBodyItems.size());

   mEngineButtonsWidth      = getButtonWidth(mEngineItems.address(), mEngineItems.size());
   mEngineItemsDisplayWidth = getMaxItemWidth(mEngineItems.address(), mEngineItems.size());

   mTreadButtonsWidth      = getButtonWidth(mTreadItems.address(), mTreadItems.size());
   mTreadItemsDisplayWidth = getMaxItemWidth(mTreadItems.address(), mTreadItems.size());

   mArmorButtonsWidth      = getButtonWidth(mArmorItems.address(), mArmorItems.size());
   mArmorItemsDisplayWidth = getMaxItemWidth(mArmorItems.address(), mArmorItems.size());

   mArmorSidesButtonsWidth      = getButtonWidth(mArmorSidesItems.address(), mArmorSidesItems.size());
   mArmorSidesItemsDisplayWidth = getMaxItemWidth(mArmorSidesItems.address(), mArmorSidesItems.size());

   mSuspensionButtonsWidth      = getButtonWidth(mSuspensionItems.address(), mSuspensionItems.size());
   mSuspensionItemsDisplayWidth = getMaxItemWidth(mSuspensionItems.address(), mSuspensionItems.size());

   mBumperButtonsWidth      = getButtonWidth(mBumperItems.address(), mBumperItems.size());
   mBumperItemsDisplayWidth = getMaxItemWidth(mBumperItems.address(), mBumperItems.size());

   mSpecialsButtonsWidth      = getButtonWidth(mSpecialsItems.address(), mSpecialsItems.size());
   mSpecialsItemsDisplayWidth = getMaxItemWidth(mSpecialsItems.address(), mSpecialsItems.size());

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
   if(mPhase == PHASE_BODY)           updateItemColors(mBodyItems);
   else if(mPhase == PHASE_ENGINE)    updateItemColors(mEngineItems);
   else if(mPhase == PHASE_TREADS)    updateItemColors(mTreadItems);
   else if(mPhase == PHASE_ARMOR)     updateItemColors(mArmorItems);
   else if(mPhase == PHASE_ARMOR_SIDES) updateItemColors(mArmorSidesItems);
   else if(mPhase == PHASE_SUSPENSION)updateItemColors(mSuspensionItems);
   else if(mPhase == PHASE_BUMPERS)   updateItemColors(mBumperItems);
   else if(mPhase == PHASE_SPECIALS)  { /* active state handled in renderCard */ }
   else if(mPhase == PHASE_HEATSINK)  updateItemColors(mHeatSinkItems);
   else                               updateItemColors(mWeaponItems);

   // Compute transition fraction (0.0 to 1.0)
   F32 transitionFraction = 0.0f;
   if(mTransitionTimer.getCurrent() > 0)
      transitionFraction = MIN(1.0f, mTransitionTimer.getFraction());

   renderFloatingMenus(transitionFraction);
   renderPreviewPanel();
}


void UIXtankHelper::idle(U32 delta)
{
   Parent::idle(delta);
   if(mTransitionTimer.getCurrent() > 0)
      mTransitionTimer.update(delta);
}


// Commit whatever is currently highlighted in mPhase into mDesignInProgress.
// Called before navigating away so LEFT/RIGHT implicitly confirms the selection.
void UIXtankHelper::commitHighlightedSelection()
{
   if(mPhase == PHASE_BODY)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mBodyItems.size())
      {
         S32 bodyIdx = (S32)mBodyItems[mHighlightedIndex].itemIndex;
         // Update body and slot count only — preserve engine, treads, weapons, etc.
         mDesignInProgress.bodyIndex = (S8)bodyIdx;
         mSlotCount = xtankTurretInfos[bodyIdx].count;
         // Clamp weapons to the new slot count (extra slots become None).
         for(S32 i = mSlotCount; i < kXtankMaxWeaponSlots; i++)
            mDesignInProgress.weapons[i] = XtankWeaponNone;
         // Reset per-side armor budget for the new body.
         S32 sideDefault = (kArmorPointsPerBodySize * body_stat[bodyIdx].size) / 4;
         for(S32 i = 0; i < 4; i++)
            mDesignInProgress.armorSides[i] = (U8)sideDefault;
      }
   }
   else if(mPhase == PHASE_ENGINE)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mEngineItems.size())
         mDesignInProgress.engineType = (XtankEngine)mEngineItems[mHighlightedIndex].itemIndex;
   }
   else if(mPhase == PHASE_TREADS)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mTreadItems.size())
         mDesignInProgress.treadType = (XtankTread)mTreadItems[mHighlightedIndex].itemIndex;
   }
   else if(mPhase == PHASE_ARMOR)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mArmorItems.size())
         mDesignInProgress.armorType = (XtankArmor)mArmorItems[mHighlightedIndex].itemIndex;
   }
   else if(mPhase == PHASE_ARMOR_SIDES)
   {
      // Points are redistributed live via +/- in processInputCode; nothing to commit here.
   }
   else if(mPhase == PHASE_SUSPENSION)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mSuspensionItems.size())
         mDesignInProgress.suspensionType = (S8)mSuspensionItems[mHighlightedIndex].itemIndex;
   }
   else if(mPhase == PHASE_BUMPERS)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mBumperItems.size())
         mDesignInProgress.bumperType = (S8)mBumperItems[mHighlightedIndex].itemIndex;
   }
   else if(mPhase == PHASE_SPECIALS)
   {
      // Specials are toggled interactively in processInputCode; nothing to commit here.
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      if(mHighlightedIndex >= 0 && mHighlightedIndex < mHeatSinkItems.size())
         mDesignInProgress.heatSinkCount = (S8)(mHighlightedIndex + XtankHeatSinkMin);
   }
   else  // PHASE_WEAPONS
   {
      if(mWeaponSide >= 0 && mWeaponSide < kXtankMaxWeaponSlots &&
         mHighlightedIndex >= 0 && mHighlightedIndex < mWeaponItems.size())
      {
         if(mHighlightedIndex == 0)
            mDesignInProgress.weapons[mWeaponSide] = XtankWeaponNone;
         else
            mDesignInProgress.weapons[mWeaponSide] = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;
      }
   }
}


// Return true if the key was handled.
bool UIXtankHelper::processInputCode(InputCode inputCode)
{
   // ESC: restore the original design and exit without applying changes.
   if(inputCode == KEY_ESCAPE || inputCode == BUTTON_DPAD_LEFT || inputCode == BUTTON_BACK)
   {
      GameUserInterface *gui = getGame()->getUIManager()->getUI<GameUserInterface>();
      if(gui)
         gui->applyXtankDesign(mOriginalDesign);
      exitHelper();
      return true;
   }

   // Let the parent handle any other cancel keys it knows about.
   if(Parent::processInputCode(inputCode))
      return true;

   // ENTER: commit current selection and finalize the design.
   if(inputCode == KEY_ENTER)
   {
      if(mPhase == PHASE_SPECIALS)
      {
         // ENTER toggles the highlighted special
         if(mHighlightedIndex >= 0 && mHighlightedIndex < XtankSpecialCount)
            mDesignInProgress.specials = toggleSpecial(mDesignInProgress.specials, (XtankSpecial)mHighlightedIndex);
         return true;
      }
      commitHighlightedSelection();
      applyDesign();
      return true;
   }

   bool shiftDown = InputCodeManager::checkModifier(KEY_SHIFT);

   // TAB / SHIFT-TAB: always navigate the phase carousel (forward / backward).
   if(inputCode == KEY_TAB)
   {
      commitHighlightedSelection();
      if(shiftDown)
         navigateBackward();
      else
         navigateForward();
      return true;
   }

   // LEFT / BACKSPACE:
   //   • In PHASE_WEAPONS with multiple slots: cycle to the previous weapon slot (side).
   //   • Everywhere else: carousel backward (same as Shift-Tab).
   if(inputCode == KEY_LEFT || inputCode == KEY_BACKSPACE)
   {
      if(mPhase == PHASE_WEAPONS && mSlotCount > 1)
      {
         commitHighlightedSelection();
         mWeaponSide = (mWeaponSide - 1 + mSlotCount) % mSlotCount;
         setHighlightedIndexForPhase(PHASE_WEAPONS);
      }
      else
      {
         commitHighlightedSelection();
         navigateBackward();
      }
      return true;
   }

   // RIGHT:
   //   • In PHASE_WEAPONS with multiple slots: cycle to the next weapon slot (side).
   //   • Everywhere else: carousel forward (same as Tab).
   if(inputCode == KEY_RIGHT)
   {
      if(mPhase == PHASE_WEAPONS && mSlotCount > 1)
      {
         commitHighlightedSelection();
         mWeaponSide = (mWeaponSide + 1) % mSlotCount;
         setHighlightedIndexForPhase(PHASE_WEAPONS);
      }
      else
      {
         commitHighlightedSelection();
         navigateForward();
      }
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
   }

   // Hotkey presses: commit the specific item then animate forward.
   if(mPhase == PHASE_BODY)
   {
      for(S32 i = 0; i < mBodyItems.size(); i++)
      {
         if(inputCode == mBodyItems[i].key)
         {
            S32 bodyIdx = (S32)mBodyItems[i].itemIndex;
            // Update body only — preserve other components.
            mDesignInProgress.bodyIndex = (S8)bodyIdx;
            mSlotCount = xtankTurretInfos[bodyIdx].count;
            for(S32 j = mSlotCount; j < kXtankMaxWeaponSlots; j++)
               mDesignInProgress.weapons[j] = XtankWeaponNone;
            // Reset armor budget for the new body.
            S32 sideDefault = (kArmorPointsPerBodySize * body_stat[bodyIdx].size) / 4;
            for(S32 k = 0; k < 4; k++)
               mDesignInProgress.armorSides[k] = (U8)sideDefault;
            mHighlightedIndex = i;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_ARMOR_SIDES)
   {
      // '+' / '=' : move 1 point from another side to the highlighted side.
      if(inputCode == KEY_EQUALS)
      {
         S32 to = mHighlightedIndex;
         for(S32 k = 1; k <= 3; k++)
         {
            S32 from = (to + k) % 4;
            if(mDesignInProgress.armorSides[from] > 0)
            {
               mDesignInProgress.armorSides[from]--;
               mDesignInProgress.armorSides[to]++;
               break;
            }
         }
         return true;
      }
      // '-' : move 1 point from the highlighted side to the next side.
      if(inputCode == KEY_MINUS)
      {
         S32 from = mHighlightedIndex;
         if(mDesignInProgress.armorSides[from] > 0)
         {
            S32 to = (from + 1) % 4;
            mDesignInProgress.armorSides[from]--;
            mDesignInProgress.armorSides[to]++;
         }
         return true;
      }
   }
   else if(mPhase == PHASE_ENGINE)
   {
      for(S32 e = 0; e < mEngineItems.size(); e++)
      {
         if(inputCode == mEngineItems[e].key)
         {
            mDesignInProgress.engineType = (XtankEngine)mEngineItems[e].itemIndex;
            mHighlightedIndex = e;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_TREADS)
   {
      for(S32 t = 0; t < mTreadItems.size(); t++)
      {
         if(inputCode == mTreadItems[t].key)
         {
            mDesignInProgress.treadType = (XtankTread)mTreadItems[t].itemIndex;
            mHighlightedIndex = t;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_ARMOR)
   {
      for(S32 a = 0; a < mArmorItems.size(); a++)
      {
         if(inputCode == mArmorItems[a].key)
         {
            mDesignInProgress.armorType = (XtankArmor)mArmorItems[a].itemIndex;
            mHighlightedIndex = a;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_SUSPENSION)
   {
      for(S32 s = 0; s < mSuspensionItems.size(); s++)
      {
         if(inputCode == mSuspensionItems[s].key)
         {
            mDesignInProgress.suspensionType = (S8)mSuspensionItems[s].itemIndex;
            mHighlightedIndex = s;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_BUMPERS)
   {
      for(S32 b = 0; b < mBumperItems.size(); b++)
      {
         if(inputCode == mBumperItems[b].key)
         {
            mDesignInProgress.bumperType = (S8)mBumperItems[b].itemIndex;
            mHighlightedIndex = b;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_SPECIALS)
   {
      for(S32 s = 0; s < mSpecialsItems.size(); s++)
      {
         if(inputCode == mSpecialsItems[s].key)
         {
            mDesignInProgress.specials = toggleSpecial(mDesignInProgress.specials, (XtankSpecial)s);
            mHighlightedIndex = s;
            return true;
         }
      }
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      for(S32 n = 0; n < mHeatSinkItems.size(); n++)
      {
         if(inputCode == mHeatSinkItems[n].key)
         {
            mDesignInProgress.heatSinkCount = (S8)(n + XtankHeatSinkMin);
            mHighlightedIndex = n;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_WEAPONS)
   {
      // Hotkeys assign a weapon to the current side/slot and advance to the next slot,
      // or navigate forward if the last slot has been assigned.
      for(S32 i = 0; i < mWeaponItems.size(); i++)
      {
         if(inputCode == mWeaponItems[i].key)
         {
            if(i == 0)
               mDesignInProgress.weapons[mWeaponSide] = XtankWeaponNone;
            else
               mDesignInProgress.weapons[mWeaponSide] = (XtankWeapon)mWeaponItems[i].itemIndex;
            mHighlightedIndex = i;
            // Advance to next slot, or leave weapon phase if last slot done.
            if(mSlotCount > 1 && mWeaponSide < mSlotCount - 1)
            {
               mWeaponSide++;
               setHighlightedIndexForPhase(PHASE_WEAPONS);
            }
            else
               navigateForward();
            return true;
         }
      }
   }

   return false;
}


void UIXtankHelper::advanceToNextPhaseOrFinish()
{
   // Now just advances — design is only applied when player presses ENTER.
   navigateForward();
}


void UIXtankHelper::navigateForward()
{
   // All weapon slots are now handled within the single PHASE_WEAPONS phase.
   // Start transition animation
   mTransitionFromPhase = mPhase;
   mTransitionForward = true;
   mTransitionTimer.reset(250);  // 250ms transition

   mPhase = (mPhase + 1) % TOTAL_PHASES;

   // When entering the weapons phase (from any other phase), start at slot 0.
   if(mPhase == PHASE_WEAPONS)
      mWeaponSide = 0;

   S32 newWidth = widthForPhase(mPhase);
   setExpectedWidth_MidTransition(newWidth);
   resetScrollTimer();
   setHighlightedIndexForPhase(mPhase);
}


// Move to the previous phase, restoring the highlighted index from the
// existing design so the player sees their previous choice pre-selected.
void UIXtankHelper::navigateBackward()
{
   // Start transition animation
   mTransitionFromPhase = mPhase;
   mTransitionForward = false;
   mTransitionTimer.reset(250);  // 250ms transition

   mPhase = (mPhase - 1 + TOTAL_PHASES) % TOTAL_PHASES;

   // When entering the weapons phase from another phase, start at the last slot
   // so backward navigation feels natural (most-recently assigned slot).
   if(mPhase == PHASE_WEAPONS)
      mWeaponSide = MAX(0, mSlotCount - 1);

   S32 newWidth = widthForPhase(mPhase);
   setExpectedWidth_MidTransition(newWidth);
   resetScrollTimer();
   setHighlightedIndexForPhase(mPhase);
}


// Returns the display width for the helper panel at the given phase.
S32 UIXtankHelper::widthForPhase(S32 phase) const
{
   if(phase == PHASE_BODY)        return getTotalDisplayWidth(mBodyButtonsWidth,        mBodyItemsDisplayWidth);
   if(phase == PHASE_ENGINE)      return getTotalDisplayWidth(mEngineButtonsWidth,      mEngineItemsDisplayWidth);
   if(phase == PHASE_TREADS)      return getTotalDisplayWidth(mTreadButtonsWidth,       mTreadItemsDisplayWidth);
   if(phase == PHASE_ARMOR)       return getTotalDisplayWidth(mArmorButtonsWidth,       mArmorItemsDisplayWidth);
   if(phase == PHASE_ARMOR_SIDES) return getTotalDisplayWidth(mArmorSidesButtonsWidth,  mArmorSidesItemsDisplayWidth);
   if(phase == PHASE_SUSPENSION)  return getTotalDisplayWidth(mSuspensionButtonsWidth,  mSuspensionItemsDisplayWidth);
   if(phase == PHASE_BUMPERS)     return getTotalDisplayWidth(mBumperButtonsWidth,      mBumperItemsDisplayWidth);
   if(phase == PHASE_SPECIALS)    return getTotalDisplayWidth(mSpecialsButtonsWidth,    mSpecialsItemsDisplayWidth);
   if(phase == PHASE_HEATSINK)    return getTotalDisplayWidth(mHeatSinkButtonsWidth,    mHeatSinkItemsDisplayWidth);
   return getTotalDisplayWidth(mWeaponButtonsWidth, mWeaponItemsDisplayWidth);
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
   else if(phase == PHASE_ARMOR)
   {
      mHighlightedIndex = (S32)mDesignInProgress.armorType;
   }
   else if(phase == PHASE_ARMOR_SIDES)
   {
      // Default to Front (index 0) when entering; preserve if already valid.
      mHighlightedIndex = 0;
   }
   else if(phase == PHASE_SUSPENSION)
   {
      mHighlightedIndex = (S32)mDesignInProgress.suspensionType;
   }
   else if(phase == PHASE_BUMPERS)
   {
      mHighlightedIndex = (S32)mDesignInProgress.bumperType;
   }
   else if(phase == PHASE_SPECIALS)
   {
      // Position cursor on the first active special, or 0 if none active.
      mHighlightedIndex = 0;
      for(S32 i = 0; i < XtankSpecialCount; i++)
      {
         if(hasSpecial(mDesignInProgress.specials, (XtankSpecial)i))
         {
            mHighlightedIndex = i;
            break;
         }
      }
   }
   else if(phase == PHASE_HEATSINK)
   {
      mHighlightedIndex = (S32)mDesignInProgress.heatSinkCount - XtankHeatSinkMin;
   }
   else  // PHASE_WEAPONS: read from mWeaponSide (caller sets mWeaponSide before calling)
   {
      S32 slot = mWeaponSide;
      if(slot < 0 || slot >= (S32)(sizeof(mDesignInProgress.weapons) / sizeof(mDesignInProgress.weapons[0])))
      {
         mHighlightedIndex = 0;
         return;
      }
      XtankWeapon w = mDesignInProgress.weapons[slot];
      if(w == XtankWeaponNone)
      {
         mHighlightedIndex = 0;
         return;
      }
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
   if(phase == PHASE_BODY)        return &mBodyItems;
   if(phase == PHASE_ENGINE)      return &mEngineItems;
   if(phase == PHASE_TREADS)      return &mTreadItems;
   if(phase == PHASE_ARMOR)       return &mArmorItems;
   if(phase == PHASE_ARMOR_SIDES) return &mArmorSidesItems;
   if(phase == PHASE_SUSPENSION)  return &mSuspensionItems;
   if(phase == PHASE_BUMPERS)     return &mBumperItems;
   if(phase == PHASE_SPECIALS)    return &mSpecialsItems;
   if(phase == PHASE_HEATSINK)    return &mHeatSinkItems;
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
   if(phase == PHASE_ARMOR)
   {
      S32 idx = (S32)mDesignInProgress.armorType;
      if(idx < 0 || idx >= XtankArmorCount) idx = 0;
      return xtankArmorInfos[idx].name;
   }
   if(phase == PHASE_ARMOR_SIDES)
   {
      // Show a compact "F:x B:x L:x R:x" summary.
      char buf[32];
      dSprintf(buf, sizeof(buf), "F:%d B:%d L:%d R:%d",
               (S32)mDesignInProgress.armorSides[0],
               (S32)mDesignInProgress.armorSides[1],
               (S32)mDesignInProgress.armorSides[2],
               (S32)mDesignInProgress.armorSides[3]);
      return std::string(buf);
   }
   if(phase == PHASE_SUSPENSION)
   {
      S32 idx = (S32)mDesignInProgress.suspensionType;
      if(idx < 0 || idx >= XtankSuspensionCount) idx = 0;
      return suspensionStat[idx].name;
   }
   if(phase == PHASE_BUMPERS)
   {
      S32 idx = (S32)mDesignInProgress.bumperType;
      if(idx < 0 || idx >= XtankBumperCount) idx = 0;
      return bumperStat[idx].name;
   }
   if(phase == PHASE_SPECIALS)
   {
      S32 count = 0;
      for(S32 i = 0; i < XtankSpecialCount; i++)
         if(hasSpecial(mDesignInProgress.specials, (XtankSpecial)i))
            count++;
      return count == 0 ? "None" : itos(count) + " selected";
   }
   if(phase == PHASE_HEATSINK)
   {
      S32 n = (S32)mDesignInProgress.heatSinkCount;
      if(n < XtankHeatSinkMin) n = XtankHeatSinkMin;
      if(n > XtankHeatSinkMax) n = XtankHeatSinkMax;
      return std::string("Heat Sinks: ") + itos(n);
   }
   if(phase == PHASE_WEAPONS)
   {
      // Show a summary of how many weapon slots are filled.
      S32 filled = 0;
      for(S32 i = 0; i < mSlotCount && i < kXtankMaxWeaponSlots; i++)
         if(mDesignInProgress.weapons[i] != XtankWeaponNone) filled++;
      return itos(filled) + "/" + itos(mSlotCount) + " armed";
   }
   return "";
}


// Render detailed stats for the highlighted item in a vertical column.
// alpha: 0.0 = invisible, 1.0 = fully opaque (used during card transitions).
void UIXtankHelper::renderItemStatsColumn(S32 left, S32 right, S32 yTop, F32 alpha) const
{
   Renderer &r = Renderer::get();
   static const S32 STAT_SZ = 11;
   static const S32 GAP = STAT_SZ + 5;
   S32 y = yTop;

   r.setColor(Color(0.7f * alpha, 0.7f * alpha, 0.7f * alpha));  // gray70 * alpha
   drawString(left, y, STAT_SZ, "Stats");
   y += GAP + 4;

   r.setColor(Color(0.0f, alpha, alpha));  // cyan * alpha
   if(mPhase == PHASE_BODY)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankBodyCount) idx = 0;
      const TankPhysicsInfo &phy = xtankPhysicsInfos[idx];
      drawStringf(left, y, STAT_SZ, "Speed: %s", cs(comma(phy.maxSpeed)));           y += GAP;
      drawStringf(left, y, STAT_SZ, "Reverse: %s", cs(comma(phy.maxReverseSpeed)));  y += GAP;
      drawStringf(left, y, STAT_SZ, "Accel: %s", cs(comma(phy.acceleration)));       y += GAP;
      drawStringf(left, y, STAT_SZ, "Turn: %.1f", phy.turnRate);                     y += GAP;
      drawStringf(left, y, STAT_SZ, "Armor: %.2f", phy.armor);                       y += GAP;
      drawStringf(left, y, STAT_SZ, "Turrets: %d", xtankTurretInfos[idx].count);     y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(body_stat[idx].weight))); y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(body_stat[idx].cost)));     y += GAP;
   }
   else if(mPhase == PHASE_ENGINE)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankEngineCount) idx = 0;
      const XtankEngineInfo &ei = xtankEngineInfos[idx];
      drawStringf(left, y, STAT_SZ, "Speed: %+s%%", cs(cs(comma((ei.speedMult - 1.0f) * 100.0f + 0.5f)))); y += GAP;
      drawStringf(left, y, STAT_SZ, "Accel: %+s%%", cs(cs(comma((ei.accelMult - 1.0f) * 100.0f + 0.5f)))); y += GAP;
      drawStringf(left, y, STAT_SZ, "Power: %s", cs(comma(ei.power)));                                     y += GAP;
      drawStringf(left, y, STAT_SZ, "Fuel: %s", cs(comma(ei.fuel)));                                       y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(ei.weight)));                                   y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(ei.cost)));                                       y += GAP;
   }
   else if(mPhase == PHASE_TREADS)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankTreadCount)
         idx = 0;
      const XtankTreadInfo &ti = xtankTreadInfos[idx];
      drawStringf(left, y, STAT_SZ, "Handling: %.2f", ti.friction);  y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(ti.cost))); y += GAP;
   }
   else if(mPhase == PHASE_ARMOR)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankArmorCount)
         idx = 0;
      const XtankArmorInfo &ai = xtankArmorInfos[idx];
      drawStringf(left, y, STAT_SZ, "Defense: %d", ai.defense);           y += GAP;
      drawStringf(left, y, STAT_SZ, "Wt/unit: %d", ai.weight);            y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost/unit: %s", cs(comma(ai.cost))); y += GAP;
   }
   else if(mPhase == PHASE_ARMOR_SIDES)
   {
      S32 total = 0;
      for(S32 i = 0; i < 4; i++)
         total += (S32)mDesignInProgress.armorSides[i];
      drawStringf(left, y, STAT_SZ, "Budget: %d pts", total); y += GAP;
      r.setColor(Color(0.7f * alpha, 0.7f * alpha, 0.7f * alpha));
      drawString (left, y, STAT_SZ, "+/- to shift pts"); y += GAP;
      r.setColor(Color(0.0f, alpha, alpha));
      const char *sideLabels[4] = { "Front", "Back", "Left", "Right" };
      for(S32 i = 0; i < 4; i++)
      {
         bool hl = (i == mHighlightedIndex);
         r.setColor(hl ? Colors::yellow : Color(0.0f, alpha, alpha));
         drawStringf(left, y, STAT_SZ, "%s: %d", sideLabels[i],
                     (S32)mDesignInProgress.armorSides[i]);
         y += GAP;
      }
   }
   else if(mPhase == PHASE_SUSPENSION)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankSuspensionCount)
         idx = 0;
      const SuspensionStat &si = suspensionStat[idx];
      drawStringf(left, y, STAT_SZ, "Handling: %+d", (S32)si.friction); y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(si.cost)));    y += GAP;
   }
   else if(mPhase == PHASE_BUMPERS)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankBumperCount)
         idx = 0;
      const BumperStat &bi = bumperStat[idx];
      drawStringf(left, y, STAT_SZ, "Elasticity: %.2f", bi.elasticity); y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(bi.cost)));    y += GAP;
   }
   else if(mPhase == PHASE_SPECIALS)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankSpecialCount) idx = 0;
      const XtankSpecialInfo &si = xtankSpecialInfos[idx];
      bool active = hasSpecial(mDesignInProgress.specials, (XtankSpecial)idx);
      r.setColor(active ? Colors::green : Colors::overlayMenuUnselectedItemColor);
      drawString(left, y, STAT_SZ, active ? "ACTIVE" : "inactive");  y += GAP;
      r.setColor(Color(0.0f, alpha, alpha));
      drawStringf(left, y, STAT_SZ, "Weight: %d", si.weight);  y += GAP;
      drawStringf(left, y, STAT_SZ, "Space:  %d", si.space);   y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost:   %s", cs(comma(si.cost)));  y += GAP;
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      S32 n = mHighlightedIndex + XtankHeatSinkMin;
      if(n < XtankHeatSinkMin) n = XtankHeatSinkMin;
      if(n > XtankHeatSinkMax) n = XtankHeatSinkMax;
      drawStringf(left, y, STAT_SZ, "Count: %d", n);                                                                  y += GAP;
      drawStringf(left, y, STAT_SZ, "Fire rate +%d%%", S32((1.0f - xtankHeatSinkFireDelayMult(n)) * 100.0f + 0.5f)); y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(heatSinkStat.weight * n)));                                y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(heatSinkStat.cost * n)));                                    y += GAP;
   }
   else
   {
      // Weapon slot
      S32 idx = mHighlightedIndex;
      if(idx <= 0)
         drawString(left, y, STAT_SZ, "Empty slot");
      else
      {
         S32 w = idx - 1;
         if(w < 0 || w >= XtankWeaponCount) w = 0;
         const XtankWeaponInfo &wi = xtankWeaponInfos[w];
         drawStringf(left, y, STAT_SZ, "Delay: %sms", cs(comma(xtankFireDelayMs(wi))));    y += GAP;
         drawStringf(left, y, STAT_SZ, "Energy: %s", cs(comma(wi.heat)));                  y += GAP;
         drawStringf(left, y, STAT_SZ, "Speed: %sdu/s", cs(comma(xtankProjVelocity(wi)))); y += GAP;
         drawStringf(left, y, STAT_SZ, "Life: %sms", cs(comma(xtankProjLiveTime(wi))));    y += GAP;
         drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(wi.weight)));                y += GAP;
         drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(wi.cost)));                    y += GAP;
      }
   }
}


// Render one card.
// centerFraction: 0.0 = fully background (adjacent), 1.0 = fully foreground (center).
void UIXtankHelper::renderCard(S32 left, S32 top, S32 right, S32 bot, S32 phase, F32 cf) const
{
   static const S32 CORNER    = 10;
   static const S32 ITEM_SZ   = 13;   // fixed item font size, same on ALL cards
   static const S32 TITLE_SZ  = 16;   // fixed title font size, same on ALL cards
   // Fixed row gap used on the center card.  Background cards scale proportionally.
   static const S32 CTR_ROW_GAP  = 16;
   // Approximate available list height on the center card (matches SLOT C geometry: bot=595, top=135).
   static const S32 CTR_LIST_AVAIL = 595 - 6 - (135 + 8 + TITLE_SZ + 4 + 6);  // 595-6-169 = 420

   // Phase title — one entry per non-weapon phase; PHASE_WEAPONS gets a single title.
   // Non-weapon phase titles only (0..PHASE_HEATSINK). PHASE_WEAPONS uses fallback below.
   static const char *sPhaseTitles[] =
   {
      "Body",         // 0
      "Engine",       // 1
      "Treads",       // 2
      "Armor Type",   // 3
      "Armor Sides",  // 4 (point allocation)
      "Suspension",   // 5
      "Bumpers",      // 6
      "Specials",     // 7
      "Heat Sinks",   // 8
   };
   auto phaseTitle = [&](S32 p) -> const char *
   {
      if(p >= 0 && p < (S32)(sizeof(sPhaseTitles) / sizeof(sPhaseTitles[0])))
         return sPhaseTitles[p];
      return "Weapon";
   };

   // Background alpha: 0.30 (distant) → 0.82 (center)
   F32 bgAlpha = 0.30f + cf * (0.82f - 0.30f);
   // Border: gray35 → red35
   Color border(0.35f, 0.35f * (1.0f - cf), 0.35f * (1.0f - cf));
   // Gray level for background cards (all text uniform gray)
   F32 grayLevel = 0.28f + cf * 0.47f;  // 0.28 (dark) → 0.75 (lighter)
   bool isCenter = (cf > 0.5f);

   Renderer &r = Renderer::get();
   drawFilledFancyBox(left, top, right, bot, CORNER, Colors::black, bgAlpha, border);

   const S32 cx = (left + right) / 2;
   const S32 w  = right - left;

   // Title
   r.setColor(isCenter ? Colors::white : Color(grayLevel, grayLevel, grayLevel));
   drawCenteredString(cx, top + 8, TITLE_SZ, phaseTitle(phase));

   // Divider
   r.setColor(Color(0.20f + cf * 0.20f, 0.20f + cf * 0.20f, 0.20f + cf * 0.20f));
   const S32 divY   = top + 8 + TITLE_SZ + 4;
   drawHorizLine(left + 8, right - 8, divY);

   const Vector<OverlayMenuItem> *items = getItemsForPhase(phase);
   if(!items || items->size() == 0)
      return;

   const S32 count   = items->size();
   S32 listTop = divY + 6;
   // Use a small fixed bottom margin; blank space accumulates below items.
   const S32 listBot = bot - 6;

   // Weapons phase (center card only): draw a slot-selector banner above the weapon list
   // so the player knows which slot they are configuring and can cycle with LEFT/RIGHT.
   if(isCenter && phase == PHASE_WEAPONS)
   {
      static char slotBuf[64];
      S32 bodyIdx = mDesignInProgress.bodyIndex;
      if(bodyIdx < 0 || bodyIdx >= XtankBodyCount) bodyIdx = 0;
      const char *sideName = getTurretSideLabel(bodyIdx, mWeaponSide);
      dSprintf(slotBuf, sizeof(slotBuf), "\x11  Slot %d / %d : %s  \x10",
               mWeaponSide + 1, mSlotCount, sideName);
      r.setColor(Colors::cyan);
      drawCenteredString(cx, listTop, ITEM_SZ, slotBuf);
      listTop += ITEM_SZ + 8;  // push the weapon list down past the banner
      r.setColor(Colors::gray40);
      drawHorizLine(left + 8, right - 8, listTop - 3);
   }

   const S32 avail   = listBot - listTop;

   // Center card: fixed CTR_ROW_GAP so all menus look the same.
   // Background card: scale the gap proportionally to its available height.

   const S32 rowGap  = isCenter ? CTR_ROW_GAP
                                : MAX(8, CTR_ROW_GAP * avail / MAX(CTR_LIST_AVAIL, 1));
   // Font size scales with rowGap but is capped at ITEM_SZ.
   const S32 drawnSz = MIN(ITEM_SZ, MAX(7, rowGap - 3));

   // Selection bar
   const S32 BAR_HEIGHT = 18;
   const S32 BAR_WIDTH = 3;
   const S32 BAR_MARGIN_R = 6;      // Space between selection bar and key code
   const S32 BAR_MARGIN_L = 3;      // Space between edge of box and selection bar
   const S32 KEY_COL_WIDTH = 10;

   // Column positions
   const S32 KEY_X     = left + BAR_WIDTH + BAR_MARGIN_R;
   const S32 NAME_X    = isCenter ? (left + 30) : (left + 6);
   const S32 STATS_DIV = left + w * 54 / 100;

   for(S32 i = 0; i < count; i++)
   {
      S32 y = listTop + i * rowGap;
      // Stop drawing if we've run out of space (items after this will be blank).
      if(y + drawnSz > listBot)
         break;

      const char *name = (*items)[i].name;
      if(!name) name = "";

      // For specials phase, prefix with toggle state indicator
      static char sSpecialNameBuf[XtankSpecialCount][64];
      if(phase == PHASE_SPECIALS && i < XtankSpecialCount)
      {
         bool active = hasSpecial(mDesignInProgress.specials, (XtankSpecial)i);
         dSprintf(sSpecialNameBuf[i], sizeof(sSpecialNameBuf[i]), "%s %s",
                  active ? "[X]" : "[ ]", name);
         name = sSpecialNameBuf[i];
      }

      // For armor sides phase, append point count to each side name.
      static char sArmorSidesBuf[4][32];
      if(phase == PHASE_ARMOR_SIDES && i < 4)
      {
         dSprintf(sArmorSidesBuf[i], sizeof(sArmorSidesBuf[i]), "%-6s %d pts",
                  name, (S32)mDesignInProgress.armorSides[i]);
         name = sArmorSidesBuf[i];
      }

      bool sel = isCenter && (i == mHighlightedIndex);

      if(isCenter)
      {
         const Color SELECTED_COLOR = Colors::yellow;
         if(sel)
         {
            // Yellow bar: vertically centered on the text with 3px padding each side
            S32 barTop = y + drawnSz / 2 - (drawnSz / 2 + 3) + 2;
            r.setColor(SELECTED_COLOR);
            drawFilledRect(left + BAR_MARGIN_L, barTop, left + BAR_MARGIN_L + BAR_WIDTH, barTop + BAR_HEIGHT);
         }
         // Key badge
         r.setColor(sel ? SELECTED_COLOR : Colors::gray60);
         drawStringc(KEY_X + KEY_COL_WIDTH / 2, y + drawnSz, drawnSz, InputCodeManager::inputCodeToString((*items)[i].key));
         // Item name
         r.setColor(sel ? SELECTED_COLOR : Colors::overlayMenuUnselectedItemColor);
         drawString(NAME_X, y, drawnSz, name);
      }
      else
      {
         // Background card: all gray, no key column, proportionally scaled font
         r.setColor(Color(grayLevel, grayLevel, grayLevel));
         drawString(NAME_X, y, drawnSz, name);
      }
   }

   // Stats column — center card only, fades in
   if(cf > 0.55f)
   {
      F32 statAlpha = (cf - 0.55f) / 0.45f;
      renderItemStatsColumn(STATS_DIV, right - 6, listTop, statAlpha);
   }

   // Navigation hint — center card only
   if(isCenter)
   {
      r.setColor(Colors::gray60);
      if(phase == PHASE_ARMOR_SIDES)
         drawCenteredString(cx, bot - 14, 10, "Up/Dn=side  +/-=pts  Tab=next  Esc=cancel");
      else
         drawCenteredString(cx, bot - 14, 10, "\x11 \x10  Tab  Enter=confirm  Esc=cancel");
   }
}


void UIXtankHelper::renderFloatingMenus(F32 /*unused*/)
{
   // -------------------------------------------------------------------------
   // Slot geometry table.  5 slots: L2(off) L1(adj) C(center) R1(adj) R2(off).
   // "tuck" factor: adjacent cards start a few pixels lower and smaller,
   // giving the impression they are physically behind the center card.
   //
   //   Slot  cx    half_w  top   bot   centerFraction
   //   L2    -80     55    210   500   0.0   (off-screen, invisible)
   //   L1     90     80    185   525   0.0   (tucked behind left edge of center)
   //   C     375    190    135   565   1.0   (full foreground)
   //   R1    660     80    185   525   0.0   (tucked behind right edge of center)
   //   R2    910     55    210   500   0.0   (off-screen, invisible)
   //
   // centerFraction also animates: 0 when in adjacent/off slot, 1 when in center.
   // -------------------------------------------------------------------------
   struct SlotGeom
   {
      S32 cx;
      S32 half_w;
      S32 top;
      S32 bot;
      F32 cf;    // centerFraction for this slot
   };

   static const SlotGeom SLOTS[5] =
   {
      { -80,  55,  210, 520,  0.0f },  // 0 = L2  off-screen
      {  90,  80,  185, 545,  0.0f },  // 1 = L1  adjacent left  (tucked)
      { 375, 190,  135, 595,  1.0f },  // 2 = C   center
      { 660,  80,  185, 545,  0.0f },  // 3 = R1  adjacent right (tucked)
      { 910,  55,  210, 520,  0.0f },  // 4 = R2  off-screen
   };

   const S32 totalPhases = TOTAL_PHASES;

   // Smoothstepped elapsed fraction: 0 at start of transition, 1 at end.
   F32 t = 0.0f;
   if(mTransitionTimer.getCurrent() > 0)
   {
      F32 e = 1.0f - mTransitionTimer.getFraction();
      t = e * e * (3.0f - 2.0f * e);
   }

   // Build the 5 cards shown this frame.
   // "toSlot" is where each card ends up; "fromSlot" is where it starts.
   // phaseAtSlot[i]: which phase lives in the i-th slot in the "to" arrangement.
   S32 phaseAtSlot[5];
   for(S32 i = 0; i < 5; i++)
      phaseAtSlot[i] = (mPhase + (i - 2) + totalPhases * 10) % totalPhases;

   struct CardAnim { S32 phase; S32 fromSlot; S32 toSlot; };
   CardAnim cards[5];

   if(mTransitionTimer.getCurrent() > 0 && mTransitionFromPhase >= 0)
   {
      if(mTransitionForward)
         for(S32 i = 0; i < 5; i++)
         { cards[i] = { phaseAtSlot[i], MIN(i + 1, 4), i }; }
      else
         for(S32 i = 0; i < 5; i++)
         { cards[i] = { phaseAtSlot[i], MAX(i - 1, 0), i }; }
   }
   else
      for(S32 i = 0; i < 5; i++)
         cards[i] = { phaseAtSlot[i], i, i };

   FontManager::pushFontContext(HelperMenuContext);

   // Draw back-to-front so center card is always on top.
   static const S32 drawOrder[5] = { 0, 4, 1, 3, 2 };
   for(S32 di = 0; di < 5; di++)
   {
      S32 ci = drawOrder[di];
      const CardAnim &ca = cards[ci];
      const SlotGeom &s0 = SLOTS[ca.fromSlot];
      const SlotGeom &s1 = SLOTS[ca.toSlot];

      S32 cx     = s0.cx     + (S32)((s1.cx     - s0.cx)     * t);
      S32 half_w = s0.half_w + (S32)((s1.half_w - s0.half_w) * t);
      S32 top    = s0.top    + (S32)((s1.top    - s0.top)    * t);
      S32 bot    = s0.bot    + (S32)((s1.bot    - s0.bot)    * t);
      F32 cf     = s0.cf     + (s1.cf     - s0.cf)     * t;

      // Skip fully invisible cards
      if(cf < 0.02f && half_w < 70) continue;
      if(cx + half_w < 0 || cx - half_w > 800) continue;

      renderCard(cx - half_w, top, cx + half_w, bot, ca.phase, cf);
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
   if(mPhase == PHASE_BODY)        return mBodyItems.size();
   if(mPhase == PHASE_ENGINE)      return mEngineItems.size();
   if(mPhase == PHASE_TREADS)      return mTreadItems.size();
   if(mPhase == PHASE_ARMOR)       return mArmorItems.size();
   if(mPhase == PHASE_ARMOR_SIDES) return mArmorSidesItems.size();
   if(mPhase == PHASE_SUSPENSION)  return mSuspensionItems.size();
   if(mPhase == PHASE_BUMPERS)     return mBumperItems.size();
   if(mPhase == PHASE_SPECIALS)    return mSpecialsItems.size();
   if(mPhase == PHASE_HEATSINK)    return mHeatSinkItems.size();
   return mWeaponItems.size();
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
   static const S32 STAT_SZ    = 12;
   static const S32 LINE_GAP   = STAT_SZ + 5;
   static const S32 TITLE_Y    = PNL_TOP + 10;
   static const S32 BODY_CY    = PNL_TOP + 88;   // raised — smaller ship
   static const S32 STATS_Y    = PNL_TOP + 165;  // tighter below ship
   static const S32 BUILD_Y    = PNL_TOP + 390;
   static const S32 DOTS_Y     = PNL_TOP + 500;
   static const S32 HINT_Y     = PNL_TOP + 520;
   static const F32 BODY_SCALE = 1.5f;            // was 2.2f

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
   else  // PHASE_WEAPONS: preview the weapon currently highlighted for the active side
   {
      S32 slot = mWeaponSide;
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

   // Armor cost and weight: per-side armor points * per-point stats
   S32 armorIdx = MAX(0, MIN((S32)preview.armorType, XtankArmorCount - 1));
   S32 totalArmorPoints = 0;
   for(S32 i = 0; i < 4; i++)
      totalArmorPoints += preview.armorSides[i];
   totalWeight += totalArmorPoints * xtankArmorInfos[armorIdx].weight;
   totalCost   += totalArmorPoints * xtankArmorInfos[armorIdx].cost;

   // Suspension and bumper costs
   totalCost += suspensionStat[MAX(0, MIN((S32)preview.suspensionType, XtankSuspensionCount - 1))].cost;
   totalCost += bumperStat[MAX(0, MIN((S32)preview.bumperType, XtankBumperCount - 1))].cost;

   // Specials costs and weights
   for(S32 s = 0; s < XtankSpecialCount; s++)
   {
      if(hasSpecial(preview.specials, (XtankSpecial)s))
      {
         totalWeight += xtankSpecialInfos[s].weight;
         totalCost   += xtankSpecialInfos[s].cost;
      }
   }

   // Space tracking
   S32 spaceLimit = body_stat[bodyIdx].space;
   S32 totalSpace = eng.space;
   totalSpace += totalArmorPoints * xtankArmorInfos[armorIdx].space;
   totalSpace += heatSinkStat.space * heatSinks;
   for(S32 i = 0; i < slotCount && i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = preview.weapons[i];
      if(w == XtankWeaponNone || (S32)w < 0 || (S32)w >= XtankWeaponCount) continue;
      totalSpace += xtankWeaponInfos[(S32)w].space;
   }
   for(S32 s = 0; s < XtankSpecialCount; s++)
      if(hasSpecial(preview.specials, (XtankSpecial)s))
         totalSpace += xtankSpecialInfos[s].space;

   S32 weightLimit = body_stat[bodyIdx].weightLimit;
   bool overWeight = totalWeight > weightLimit;
   bool overSpace  = totalSpace  > spaceLimit;

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
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Body: %s", xtankBodyNames[bodyIdx]);                                    ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Speed: %s  Rev: %s", cs(comma(effSpeed)), cs(comma(effRev)));           ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Accel: %s  Turn: %.1f", cs(comma(effAccel)), effTurn);                  ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Armor: %.2f  Turrets: %d", phy.armor, slotCount);                       ty += LINE_GAP;
   if(fireRatePct > 0)
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Fire bonus: +%d%%", fireRatePct);
   else
      drawCenteredString(PNL_CX, ty, STAT_SZ, "Fire bonus: base");
   ty += LINE_GAP;
   r.setColor(overWeight ? Colors::red : Colors::green);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Weight: %s / %s%s",
       cs(comma(totalWeight)), cs(comma(weightLimit)),
       overWeight ? " OVER!" : ""); ty += LINE_GAP;
   r.setColor(overSpace ? Colors::red : Colors::cyan);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Space:  %s / %s%s",
       cs(comma(totalSpace)), cs(comma(spaceLimit)),
       overSpace ? " OVER!" : ""); ty += LINE_GAP;
   r.setColor(Colors::yellow);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Cost: %s", cs(comma(totalCost))); ty += LINE_GAP + 2;

   // Show weapon loadout
   r.setColor(Colors::gray45);
   drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, ty);
   ty += 6;
   r.setColor(Colors::gray70);
   drawCenteredString(PNL_CX, ty, STAT_SZ - 1, "Weapons");
   ty += LINE_GAP;
   for(S32 i = 0; i < slotCount && i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = preview.weapons[i];
      // Highlight whichever slot is being edited in PHASE_WEAPONS.
      bool activeSlot = (mPhase == PHASE_WEAPONS && mWeaponSide == i);
      S32 bodyIdx2 = preview.bodyIndex;
      if(bodyIdx2 < 0 || bodyIdx2 >= XtankBodyCount) bodyIdx2 = 0;
      const char *sideName = getTurretSideLabel(bodyIdx2, i);
      r.setColor(activeSlot ? Colors::overlayMenuSelectedItemColor : Colors::cyan);
      if(w == XtankWeaponNone || (S32)w < 0 || (S32)w >= XtankWeaponCount)
         drawCenteredStringf(PNL_CX, ty, STAT_SZ, "%s: --", sideName);
      else
         drawCenteredStringf(PNL_CX, ty, STAT_SZ, "%s: %s", sideName, xtankWeaponInfos[(S32)w].name);
      ty += LINE_GAP;
   }

   // Active specials count
   S32 specialCount = 0;
   for(S32 s = 0; s < XtankSpecialCount; s++)
      if(hasSpecial(preview.specials, (XtankSpecial)s))
         specialCount++;
   if(specialCount > 0)
   {
      r.setColor(Colors::cyan);
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Specials: %d active", specialCount); ty += LINE_GAP;
   }

   renderFullBuildStats(PNL_CX, BUILD_Y, bodyIdx, engineIdx, treadIdx, heatSinks);
   renderCarouselDots(PNL_CX, DOTS_Y);

   r.setColor(Colors::gray50);
   if(mPhase == PHASE_WEAPONS && mSlotCount > 1)
      drawCenteredStringf(PNL_CX, HINT_Y, 11, "[Lt]/[Rt] slot  [Up]/[Dn] weapon  Tab phase");
   else
      drawCenteredString(PNL_CX, HINT_Y, 11, "[Up]/[Dn] item  Tab/[Lt]/[Rt] phase");

   FontManager::popFontContext();
}

// Draw a row of small circles indicating the current carousel position.
// Since phase navigation wraps, both arrows are always active.
void UIXtankHelper::renderCarouselDots(S32 cx, S32 y) const
{
   S32 totalPhases = TOTAL_PHASES;

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
