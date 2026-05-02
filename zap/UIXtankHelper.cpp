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
#include "XtankShape.h"

#include "stringUtils.h"

#include <cmath>
#include <array>


#ifdef TNL_OS_WIN32
#  include <windows.h>     // For ARRAYSIZE
#endif

namespace Zap
{


   // For clarity and consistency with other helpers
#define UNSEL_COLOR &Colors::overlayMenuUnselectedItemColor

const Color TABLE_HEADER_COLOR = Colors::gray50;
const Color SELECTED_COLOR = Colors::yellow;
const Color UNSELECTED_ITEM_COLOR_NAME = Colors::overlayMenuUnselectedItemColor;
const Color UNSELECTED_ITEM_COLOR_DATA = Colors::overlayMenuHelpColor;

struct XtankDerivedStats
{
   F32 maxSpeed;
   F32 maxReverseSpeed;
   F32 maxAccel;
   F32 maxTurnRate;
};

//// Returns max total armor units a bare hull can carry for the specified armor type,
//// limited by both weight limit and space limit (xtank-style size scaling).
//static S32 getBodyArmorCapacityUnits(S32 bodyIdx, S32 armorIdx)
//{
//   bodyIdx = MAX(0, MIN(bodyIdx, VehicleBodyCount - 1));
//   armorIdx = MAX(0, MIN(armorIdx, XtankArmorCount - 1));
//
//   const XtankBodyInfo2 &body = xTankBodyStats[bodyIdx];
//   const XtankArmorInfo &armor = xtankArmorInfos[armorIdx];
//
//   S32 weightBudget = MAX(0, body.weightLimit - body.weight);
//   S32 unitWeight = armor.weight * body.size;
//   S32 unitSpace = armor.space * body.size;
//
//   S32 byWeight = (unitWeight > 0) ? (weightBudget / unitWeight) : S32_MAX;
//   S32 bySpace = (unitSpace > 0) ? (body.space / unitSpace) : S32_MAX;
//
//   return MAX(0, MIN(byWeight, bySpace));
//}


// Derive movement values from original xtank stats (body_stat + engine/treads).
static XtankDerivedStats getDerivedStats(const XtankDesign &design)
{
   static const F32 XTANK_FPS = 20.0f;
   static const F32 BF_SCALE = 1.5f;
   static const F32 TURN_SCALE = 3.5f;
   static const F32 MIN_EFFECTIVE_HANDLING = 1.0f;

   S32 bodyIdx = CLAMP((S32)design.body, 0, VehicleBodyCount - 1);
   S32 engineIdx = CLAMP((S32)design.engine, 0, XtankEngineCount - 1);
   S32 treadIdx = CLAMP((S32)design.tread, 0, XtankTreadCount - 1);
   S32 armorIdx = CLAMP((S32)design.armor, 0, XtankArmorCount - 1);
   S32 suspensionIdx = CLAMP((S32)design.suspension, 0, XtankSuspensionCount - 1);
   S32 heatSinks = CLAMP((S32)design.heatSinks, 0, MAX_HEAT_SINKS);

   const XtankBodyInfo2 &body = xTankBodyStats[bodyIdx];
   const XtankEngineInfo &engine = xtankEngineInfos[engineIdx];
   const XtankTreadInfo &tread = xtankTreadInfos[treadIdx];
   const XtankArmorInfo &armor = xtankArmorInfos[armorIdx];
   const SuspensionStat &suspension = suspensionStat[suspensionIdx];

   S32 weaponWeight = 0;
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      XtankWeapon w = design.weapons[i];
      if(w != XtankWeapon::NONE)
         weaponWeight += xtankWeaponInfos[(S32)w].weight;
   }

   S32 totalArmorPts = 0;
   for(S32 i = 0; i < 6; i++)
      totalArmorPts += (S32)design.armorSides[i];
   S32 armorWeight = totalArmorPts * armor.weight * body.size;

   S32 specialsWeight = 0;
   for(S32 s = 0; s < XtankSpecialCount; s++)
      if(hasSpecial(design.specials, (XtankSpecial)s))
         specialsWeight += xtankSpecialInfos[s].weight;

   S32 totalWeight = body.weight + engine.weight + weaponWeight + armorWeight +
      heatSinkStat.weight * heatSinks + specialsWeight;
   if(totalWeight < 1) totalWeight = 1;

   F32 drag = MAX(0.01f, body.drag);
   F32 treadFric = MAX(0.01f, tread.friction);
   bool isHover = (design.tread == XtankTread::HOVER);

   F32 xt_max_speed = powf((F32)engine.power / drag, 1.0f / 3.0f) / treadFric;
   if(isHover)
      xt_max_speed *= 0.5f;

   F32 xt_engine_acc = 16.0f * (F32)engine.power / (F32)totalWeight;
   F32 xt_tread_acc = isHover ? 1.0f : (treadFric * (F32)MAX_ACCEL);

   F32 effectiveHandling = MAX(MIN_EFFECTIVE_HANDLING,
      (F32)body.handling + suspension.friction);
   F32 xt_max_turn = effectiveHandling / 8.0f;

   XtankDerivedStats out;
   out.maxSpeed = xt_max_speed * XTANK_FPS * BF_SCALE;
   out.maxReverseSpeed = out.maxSpeed * 0.4f;
   out.maxAccel = MIN(xt_engine_acc, xt_tread_acc) * XTANK_FPS * XTANK_FPS * BF_SCALE;
   out.maxTurnRate = xt_max_turn * TURN_SCALE;
   return out;
}


// Keys used for the 14 bodies: 1-9, 0, A-D.
static const InputCode sBodyKeys[VehicleBodyCount] =
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
static const InputCode sHeatSinkKeys[MAX_HEAT_SINKS] =
{
   KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
};


// Keys used for heat-sink count (6 options, keys 1-6).
static const InputCode sArmorAllocationKeys[VehicleSidesCount] =
{
   KEY_F, KEY_B, KEY_L, KEY_R, KEY_T, KEY_M,
};


//// Returns a human-readable side label for a turret based on its mount position
//// in body space (+Y = forward/nose).  Thresholds are intentionally generous so
//// all layouts produce a useful label.
//const char *UIXtankHelper::getTurretSideLabel(S32 bodyIdx, S32 slot)
//{
//   if(bodyIdx < 0 || bodyIdx >= VehicleBodyCount) return "Slot";
//   const XtankBodyTurrets &bt = xtankTurretInfos[bodyIdx];
//   if(slot < 0 || slot >= bt.count) return "Slot";
//   F32 x = bt.turrets[slot].x;
//   F32 y = bt.turrets[slot].y;
//   if(fabsf(x) < 2.0f && fabsf(y) < 2.0f) return "Center";
//   if(fabsf(y) >= fabsf(x))
//	  return (y > 0.0f) ? "Front" : "Rear";
//   return (x > 0.0f) ? "Right" : "Left";
//}


static const char *getMountLabel(XtankMountLocation mount)
{
   switch(mount)
   {
   case XtankMountLocation::TURRET1: return "Turret 1";
   case XtankMountLocation::TURRET2: return "Turret 2";
   case XtankMountLocation::TURRET3: return "Turret 3";
   case XtankMountLocation::TURRET4: return "Turret 4";
   case XtankMountLocation::FRONT:   return "Front";
   case XtankMountLocation::BACK:    return "Back";
   case XtankMountLocation::LEFT:    return "Left";
   case XtankMountLocation::RIGHT:   return "Right";
   default:            return "None";
   }
}



////////////////////////////////////////
////////////////////////////////////////

// Constructor
UIXtankHelper::UIXtankHelper()
{
   mPhase = Phase::BODY;
   mWeaponSlot = 0;
   mHighlightedIndex = 0;
   mTransitionFromPhase = (Phase)0;
   mTransitioningForward = true;
   mBodyButtonsWidth = 0;
   mBodyItemsDisplayWidth = 0;
   mEngineButtonsWidth = 0;
   mEngineItemsDisplayWidth = 0;
   mTreadButtonsWidth = 0;
   mTreadItemsDisplayWidth = 0;
   mArmorButtonsWidth = 0;
   mArmorItemsDisplayWidth = 0;
   mArmorSidesButtonsWidth = 0;
   mArmorSidesItemsDisplayWidth = 0;
   mSuspensionButtonsWidth = 0;
   mSuspensionItemsDisplayWidth = 0;
   mBumperButtonsWidth = 0;
   mBumperItemsDisplayWidth = 0;
   mSpecialsButtonsWidth = 0;
   mSpecialsItemsDisplayWidth = 0;
   mHeatSinkButtonsWidth = 0;
   mHeatSinkItemsDisplayWidth = 0;
   mWeaponButtonsWidth = 0;
   mWeaponItemsDisplayWidth = 0;

   mArmorInfo = ArmorInfo();
   mEngineInfo = EngineInfo();
   mTreadInfo = TreadInfo();
   mSuspensionInfo = SuspensionInfo();
   mBumperInfo = BumperInfo();
   mWeaponInfo = WeaponInfo2();
}


// Destructor
UIXtankHelper::~UIXtankHelper()
{
   // Do nothing
}


HelperMenu::HelperMenuType UIXtankHelper::getType() { return XtankHelperType; }


// Build the body-selection menu items (used for background cards).
void UIXtankHelper::buildBodyItems()
{
   mBodyItems.clear();
   for(S32 i = 0; i < VehicleBodyCount; i++)
   {
      OverlayMenuItem item;
      item.key = sBodyKeys[i];
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)i;
      item.name = xtankBodyNames[i];
      item.itemColor = UNSEL_COLOR;
      item.help = "";
      item.helpColor = UNSEL_COLOR;
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
      item.key = getKeyForIndex(e);
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)e;
      item.name = info.name;
      item.itemColor = UNSEL_COLOR;
      item.help = engineHelpTexts[e].c_str();
      item.helpColor = UNSEL_COLOR;
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
      item.key = getKeyForIndex(t);
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)t;
      item.name = info.name;
      item.itemColor = UNSEL_COLOR;
      item.help = treadHelpTexts[t].c_str();
      item.helpColor = UNSEL_COLOR;
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
      item.key = getKeyForIndex(a);
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)a;
      item.name = info.name;
      item.itemColor = UNSEL_COLOR;
      item.help = armorHelpTexts[a].c_str();
      item.helpColor = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mArmorItems.push_back(item);
   }
}


// Build the 6-item armor-sides phase list (Front / Back / Left / Right / Top / Bottom).
// Items have no hotkeys; point values are edited live with +/- in processInputCode.
// Only called onActivated
void UIXtankHelper::buildArmorSidesItems()
{
   mArmorSidesItems.clear();
   for(S32 i = 0; i < 6; i++)
   {
      OverlayMenuItem item;
      item.key = sArmorAllocationKeys[i];
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)i;
      item.name = sideNames[i];
      item.itemColor = UNSEL_COLOR;
      item.help = "";
      item.helpColor = UNSEL_COLOR;
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
      item.key = getKeyForIndex(s);
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)s;
      item.name = info.name;
      item.itemColor = UNSEL_COLOR;
      item.help = suspHelpTexts[s].c_str();
      item.helpColor = UNSEL_COLOR;
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
      item.key = getKeyForIndex(b);
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)b;
      item.name = info.name;
      item.itemColor = UNSEL_COLOR;
      item.help = bumperHelpTexts[b].c_str();
      item.helpColor = UNSEL_COLOR;
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
      item.key = getKeyForIndex(s);
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)s;
      item.name = info.name;
      item.itemColor = UNSEL_COLOR;
      item.help = specHelpTexts[s].c_str();
      item.helpColor = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mSpecialsItems.push_back(item);
   }
}


static const InputCode hotKeys[] =
{
   KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7,
   KEY_8, KEY_9, KEY_0, KEY_A, KEY_B, KEY_C, KEY_D,
   KEY_E, KEY_F, KEY_G, KEY_H,
   KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N,
   KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
   KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
};


static S32 getMaxKeyWidth(S32 fontSize)
{
   S32 w = 0;
   for(S32 i = 0; i < sizeof(hotKeys) / sizeof(hotKeys[0]); i++)
   {
      const char *keyStr = InputCodeManager::inputCodeToString(hotKeys[i]);
      w = MAX(w, getStringWidth(fontSize, keyStr));
   }
   return w;
}


// Build the heat-sink count selection menu items (1-6).
//void UIXtankHelper::buildHeatSinkItems()
//{
//   mHeatSinkItems.clear();
//   static string hsHelpTexts[MAX_HEAT_SINKS + 1];  // persistent storage
//
//   for(S32 n = 0; n <= MAX_HEAT_SINKS; n++)
//   {
//	  S32 idx = n;
//	  S32 mult = xtankHeatSinkFireDelayMult(n);
//	  S32 pct = (S32)((1.0f - mult) * 100.0f + 0.5f);
//
//	  if(pct == 0)
//		 hsHelpTexts[idx] = "No fire-rate bonus";
//	  else
//	  {
//		 hsHelpTexts[idx] = itos(pct) + "% faster weapon cycling";
//	  }
//
//	  static char namesBuf[MAX_HEAT_SINKS][16];
//	  dSprintf(namesBuf[idx], sizeof(namesBuf[idx]), "Heat Sinks: %d", n);
//
//	  OverlayMenuItem item;
//	  item.key = sHeatSinkKeys[idx];
//	  item.button = KEY_NONE;
//	  item.showOnMenu = true;
//	  item.itemIndex = (U32)n;
//	  item.name = namesBuf[idx];
//	  item.itemColor = UNSEL_COLOR;
//	  item.help = hsHelpTexts[idx].c_str();
//	  item.helpColor = UNSEL_COLOR;
//	  item.buttonOverrideColor = NULL;
//	  mHeatSinkItems.push_back(item);
//   }
//}


// Build the weapon-selection menu items for a given slot.
void UIXtankHelper::buildWeaponItems()
{
   mWeaponItems.clear();

   // First option is always "None" (key 0).
   {
      OverlayMenuItem item;
      item.key = KEY_0;
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)XtankWeapon::NONE + 1;  // offset so 0 = None stored safely
      item.name = "None";
      item.itemColor = UNSEL_COLOR;
      item.help = "Empty slot";
      item.helpColor = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponItems.push_back(item);
   }

   // One entry per weapon (dynamically assigned keys 1-9, A-P).
   for(S32 w = 0; w < (S32)XtankWeapon::COUNT; w++)
   {
      OverlayMenuItem item;
      item.key = getKeyForIndex(w);  // Use dynamic key assignment
      item.button = KEY_NONE;
      item.showOnMenu = true;
      item.itemIndex = (U32)w;
      item.name = xtankWeaponInfos[w].name;
      item.itemColor = UNSEL_COLOR;
      item.help = "";
      item.helpColor = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponItems.push_back(item);
   }
}


//// Returns how many weapon slots (excluding `excludeSlot`) are assigned to `mount`.
//S32 UIXtankHelper::countWeaponsOnMount(XtankMountLocation mount, S32 excludeSlot) const
//{
//   S32 count = 0;
//   for(S32 i = 0; i < WEAPON_SLOTS; i++)
//   {
//	  if(i == excludeSlot)
//		 continue;
//	  if(mDesignInProgress.weapons[i] != XtankWeapon::NONE && (XtankMountLocation)mDesignInProgress.weaponMounts[i] == mount)
//		 count++;
//   }
//   return count;
//}


// Remove any "holes" in the weapon list so all selected weapons are contiguous at the front.
void UIXtankHelper::normalizeWeaponPanels()
{
   // Compact all active weapons to the front so the weapon panel sequence is
   // always: [selected weapons...] + [one trailing None], capped at 6.
   XtankWeapon compactWeapons[WEAPON_SLOTS];
   S32 compactMounts[WEAPON_SLOTS];
   S32 writeIdx = 0;

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      XtankWeapon w = mDesignInProgress.weapons[i];
      if(w == XtankWeapon::NONE)
         continue;
      compactWeapons[writeIdx] = w;
      compactMounts[writeIdx] = (S32)mDesignInProgress.weaponMounts[i];
      writeIdx++;
   }

   for(S32 i = 0; i < writeIdx; i++)
   {
      mDesignInProgress.weapons[i] = compactWeapons[i];
      mDesignInProgress.weaponMounts[i] = (XtankMountLocation)compactMounts[i];
   }
   for(S32 i = writeIdx; i < WEAPON_SLOTS; i++)
   {
      mDesignInProgress.weapons[i] = XtankWeapon::NONE;
      mDesignInProgress.weaponMounts[i] = XtankMountLocation::NONE;
   }

   //mWeaponSlot = (mWeaponSlot + 1) % WEAPON_SLOTS;  // Wrap current slot index if needed
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
   mPhase = Phase(0);	  // Start with body selection

   mHighlightedIndex = 0;
   //mWeaponSide = XtankMountLocation::NONE;

   // Pre-populate the design from the ship's current state.
   Ship *ship = getGame()->getLocalPlayerShip();
   if(ship && ship->mXtankDesign.isXtankVehicle())
      mDesignInProgress = ship->mXtankDesign;
   else
      // No existing design — start from a clean default.
      mDesignInProgress.init();

   // Snapshot so ESC can restore the original.
   mOriginalDesign = mDesignInProgress;

   // Build dynamic weapon panel count: selected weapons + one trailing empty.
   normalizeWeaponPanels();

   buildBodyItems();
   buildEngineItems();
   buildTreadItems();
   buildArmorItems();
   buildArmorSidesItems();
   buildSuspensionItems();
   buildBumperItems();
   buildSpecialsItems();
   //buildHeatSinkItems();
   buildWeaponItems();

   mBodyButtonsWidth = getButtonWidth(mBodyItems.address(), mBodyItems.size());
   mBodyItemsDisplayWidth = getMaxItemWidth(mBodyItems.address(), mBodyItems.size());

   mEngineButtonsWidth = getButtonWidth(mEngineItems.address(), mEngineItems.size());
   mEngineItemsDisplayWidth = getMaxItemWidth(mEngineItems.address(), mEngineItems.size());

   mTreadButtonsWidth = getButtonWidth(mTreadItems.address(), mTreadItems.size());
   mTreadItemsDisplayWidth = getMaxItemWidth(mTreadItems.address(), mTreadItems.size());

   mArmorButtonsWidth = getButtonWidth(mArmorItems.address(), mArmorItems.size());
   mArmorItemsDisplayWidth = getMaxItemWidth(mArmorItems.address(), mArmorItems.size());

   mArmorSidesButtonsWidth = getButtonWidth(mArmorSidesItems.address(), mArmorSidesItems.size());
   mArmorSidesItemsDisplayWidth = getMaxItemWidth(mArmorSidesItems.address(), mArmorSidesItems.size());

   mSuspensionButtonsWidth = getButtonWidth(mSuspensionItems.address(), mSuspensionItems.size());
   mSuspensionItemsDisplayWidth = getMaxItemWidth(mSuspensionItems.address(), mSuspensionItems.size());

   mBumperButtonsWidth = getButtonWidth(mBumperItems.address(), mBumperItems.size());
   mBumperItemsDisplayWidth = getMaxItemWidth(mBumperItems.address(), mBumperItems.size());

   mSpecialsButtonsWidth = getButtonWidth(mSpecialsItems.address(), mSpecialsItems.size());
   mSpecialsItemsDisplayWidth = getMaxItemWidth(mSpecialsItems.address(), mSpecialsItems.size());

   mHeatSinkButtonsWidth = getButtonWidth(mHeatSinkItems.address(), mHeatSinkItems.size());
   mHeatSinkItemsDisplayWidth = getMaxItemWidth(mHeatSinkItems.address(), mHeatSinkItems.size());

   mWeaponButtonsWidth = getButtonWidth(mWeaponItems.address(), mWeaponItems.size());
   mWeaponItemsDisplayWidth = getMaxItemWidth(mWeaponItems.address(), mWeaponItems.size());
   mWeaponMountButtonsWidth = getMaxKeyWidth(10); // TODO: Update this

   setExpectedWidth(getTotalDisplayWidth(mBodyButtonsWidth, mBodyItemsDisplayWidth));

   // Pre-select the highlighted item to match the existing design.
   mHighlightedIndex = getHighlightedIndexForPhase();

   Parent::onActivated();
}


void UIXtankHelper::render()
{
   if(mPhase == Phase::BODY)           updateItemColors(mBodyItems);
   else if(mPhase == Phase::ENGINE)    updateItemColors(mEngineItems);
   else if(mPhase == Phase::TREADS)    updateItemColors(mTreadItems);
   else if(mPhase == Phase::ARMOR)     updateItemColors(mArmorItems);
   else if(mPhase == Phase::ARMOR_SIDES) updateItemColors(mArmorSidesItems);
   else if(mPhase == Phase::SUSPENSION)updateItemColors(mSuspensionItems);
   else if(mPhase == Phase::BUMPERS)   updateItemColors(mBumperItems);
   else if(mPhase == Phase::SPECIALS) { /* active state handled in renderCard */ }
   else if(mPhase == Phase::HEATSINK)  updateItemColors(mHeatSinkItems);
   else if(mPhase == Phase::WEAPONS)   updateItemColors(mWeaponItems);
   else if(mPhase == Phase::NONE) { /* Do nothing */ }
   else TNLAssert(false, "Unknown phase");

   renderFloatingMenus();
   VehiclePreviewRenderer::renderPreviewPanel(mDesignInProgress, mPhase, mWeaponSlot);
}


void UIXtankHelper::idle(U32 delta)
{
   Parent::idle(delta);
   if(mTransitionTimer.getCurrent() > 0)
      mTransitionTimer.update(delta);
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
      if(mPhase == Phase::SPECIALS)
      {
         // ENTER toggles the highlighted special
         if(mHighlightedIndex >= 0 && mHighlightedIndex < XtankSpecialCount)
            mDesignInProgress.specials = toggleSpecial(mDesignInProgress.specials, (XtankSpecial)mHighlightedIndex);
         return true;
      }
      applyDesign();
      return true;
   }

   bool shiftDown = InputCodeManager::checkModifier(KEY_SHIFT);
   bool ctrlDown = InputCodeManager::checkModifier(KEY_CTRL);


   // LEFT/BACKSPACE and RIGHT are not used for phase navigation.
   // In the weapon phase they toggle mount choice for the current weapon panel.
   // In armor-allocation phase they mirror '-' and '+' respectively.
   if(mPhase == Phase::WEAPONS)	  // Previous mount point
   {
      if(inputCode == KEY_LEFT)
      {
         mDesignInProgress.previousMount(mWeaponSlot);
         return true;
      }
      else if(inputCode == KEY_RIGHT)
      {
         mDesignInProgress.nextMount(mWeaponSlot);
         return true;
      }

      for(S32 i = 0; i < mWeaponItems.size(); i++)
      {
         if(inputCode == mWeaponItems[i].key)
         {
            mHighlightedIndex = i;
            mDesignInProgress.selected(mPhase, mHighlightedIndex, mWeaponSlot);

            return true;
         }
      }
      // In weapon phase, Tab & Shift-Tab move between weapon-number panels
      if(inputCode == KEY_TAB)
      {
         normalizeWeaponPanels();

         if(shiftDown)	  // Shift-Tab pressed -- previous panel
         {
            bool changePhase = true;
            if(mWeaponSlot > 0)
            {
               mWeaponSlot--;
               mHighlightedIndex = getHighlightedIndexForPhase();
               changePhase = false;	   // Stay in weapon phase for another screen
            }
            //else
            navigateBackward(changePhase);
         }
         else		 // Tab pressed -- next panel
         {
            bool changePhase = true;
            // Check if out current slot is full and if there is another available to advance into
            if(mDesignInProgress.weapons[mWeaponSlot] != XtankWeapon::NONE && mWeaponSlot + 1 < WEAPON_SLOTS)
            {
               mWeaponSlot++;
               mHighlightedIndex = getHighlightedIndexForPhase();
               changePhase = false;	  // Stay in weapon phase for another screen
            }
            //else
            navigateForward(changePhase);
         }
         return true;
      }
   }

   else if(mPhase == Phase::ARMOR_SIDES)
   {
      // Secret shortcut keys to change armor on the allocation screen: ctrl+up / ctrl+down
      if(inputCode == KEY_UP && ctrlDown)
      {
         mDesignInProgress.previousArmor();
         return true;
      }

      if(inputCode == KEY_DOWN && ctrlDown)
      {
         mDesignInProgress.nextArmor();
         return true;
      }

      if(inputCode == KEY_RIGHT || inputCode == KEY_EQUALS)
      {
         if(shiftDown)
            mDesignInProgress.increaseArmor((VehicleSides)mHighlightedIndex, 10);
         else
            mDesignInProgress.increaseArmor((VehicleSides)mHighlightedIndex);

         mArmorAllocationInfo.updateArmorAllocationMenuItems(mDesignInProgress.armor, mDesignInProgress.armorSides.data());
         return true;
      }
      else if(inputCode == KEY_LEFT || inputCode == KEY_MINUS)
      {
         if(shiftDown)
            mDesignInProgress.reduceArmor((VehicleSides)mHighlightedIndex, 10);
         else
            mDesignInProgress.reduceArmor((VehicleSides)mHighlightedIndex);

         mArmorAllocationInfo.updateArmorAllocationMenuItems(mDesignInProgress.armor, mDesignInProgress.armorSides.data());
         return true;
      }

      // Hotkeys work a little different here -- they select the row but do not advance
      for(S32 i = 0; i < mArmorSidesItems.size(); i++)
      {
         if(inputCode == mArmorSidesItems[i].key)
         {
            mHighlightedIndex = i;

            if(shiftDown)
               mDesignInProgress.reduceArmor((VehicleSides)mHighlightedIndex);
            else
               mDesignInProgress.increaseArmor((VehicleSides)mHighlightedIndex);

            mArmorAllocationInfo.updateArmorAllocationMenuItems(mDesignInProgress.armor, mDesignInProgress.armorSides.data());
            return true;
         }
      }
   }

   else if(mPhase == Phase::HEATSINK)
   {
      if(inputCode == KEY_DOWN || inputCode == KEY_LEFT || inputCode == KEY_MINUS)
      {
         S32 amt = shiftDown ? 10 : 1;
         mDesignInProgress.heatSinks = MAX(mDesignInProgress.heatSinks - amt, 0);
         return true;
      }
      else if(inputCode == KEY_UP || inputCode == KEY_RIGHT || inputCode == KEY_EQUALS)
      {
         S32 amt = shiftDown ? 10 : 1;
         mDesignInProgress.heatSinks = MIN(mDesignInProgress.heatSinks + amt, MAX_HEAT_SINKS);
         return true;
      }
   }



   /// Hotkey handling: commit the specific item then animate forward

   if(mPhase == Phase::BODY)
   {
      for(S32 i = 0; i < mBodyItems.size(); i++)
      {
         if(inputCode == mBodyItems[i].key)
         {
            // Update body only — preserve other components.
            mDesignInProgress.body = (XtankBody)mBodyItems[i].itemIndex;
            mHighlightedIndex = i;
            navigateForward(true);
            return true;
         }
      }
   }
   else if(mPhase == Phase::ENGINE)
   {
      for(S32 e = 0; e < mEngineItems.size(); e++)
      {
         if(inputCode == mEngineItems[e].key)
         {
            mDesignInProgress.engine = (XtankEngine)mEngineItems[e].itemIndex;
            mHighlightedIndex = e;
            navigateForward(true);
            return true;
         }
      }
   }
   else if(mPhase == Phase::TREADS)
   {
      for(S32 t = 0; t < mTreadItems.size(); t++)
      {
         if(inputCode == mTreadItems[t].key)
         {
            mDesignInProgress.tread = (XtankTread)mTreadItems[t].itemIndex;
            mHighlightedIndex = t;
            navigateForward(true);
            return true;
         }
      }
   }
   else if(mPhase == Phase::ARMOR)
   {
      for(S32 a = 0; a < mArmorItems.size(); a++)
      {
         if(inputCode == mArmorItems[a].key)
         {
            mDesignInProgress.armor = (XtankArmor)mArmorItems[a].itemIndex;
            mHighlightedIndex = a;
            navigateForward(true);
            return true;
         }
      }
   }
   else if(mPhase == Phase::SUSPENSION)
   {
      for(S32 s = 0; s < mSuspensionItems.size(); s++)
      {
         if(inputCode == mSuspensionItems[s].key)
         {
            mDesignInProgress.suspension = (XtankSuspension)mSuspensionItems[s].itemIndex;
            mHighlightedIndex = s;
            navigateForward(true);
            return true;
         }
      }
   }
   else if(mPhase == Phase::BUMPERS)
   {
      for(S32 b = 0; b < mBumperItems.size(); b++)
      {
         if(inputCode == mBumperItems[b].key)
         {
            mDesignInProgress.bumper = (XtankBumper)mBumperItems[b].itemIndex;
            mHighlightedIndex = b;
            navigateForward(true);
            return true;
         }
      }
   }
   else if(mPhase == Phase::SPECIALS)
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
   else if(mPhase == Phase::HEATSINK)
   {
      for(S32 n = 0; n < mHeatSinkItems.size(); n++)
      {
         if(inputCode == mHeatSinkItems[n].key)
         {
            mDesignInProgress.heatSinks = n;
            mHighlightedIndex = n;
            navigateForward(true);
            return true;
         }
      }
   }


   /// Done with panel specific input handling -- now handle general navigation keys that work on all panels.

   // TAB / SHIFT-TAB: the only menu navigation.
   if(inputCode == KEY_TAB)
   {
      if(shiftDown) navigateBackward(true);		 // Shift+Tab
      else          navigateForward(true);		 // Tab

      return true;
   }


   // UP/DOWN arrows cycle through the highlighted item in the current phase.
   S32 itemCount = currentPhaseItemCount();

   if(itemCount > 0)
   {
      if(inputCode == KEY_UP)
      {
         mHighlightedIndex = (mHighlightedIndex - 1 + itemCount) % itemCount;
         mDesignInProgress.selected(mPhase, mHighlightedIndex, mWeaponSlot);
         return true;
      }
      if(inputCode == KEY_DOWN)
      {
         mHighlightedIndex = (mHighlightedIndex + 1) % itemCount;
         mDesignInProgress.selected(mPhase, mHighlightedIndex, mWeaponSlot);

         return true;
      }
   }

   return false;
}


// Show next panel
void UIXtankHelper::navigateForward(bool changePhase)
{
   mTransitionFromPhase = mPhase;   // Start transition animation
   mTransitioningForward = true;
   if(changePhase)
      mPhase = nextEnum(mPhase);

   // When entering weapon phases from non-weapon phases, reset to slot 0.
   if(mPhase == Phase::WEAPONS && mTransitionFromPhase != Phase::WEAPONS)
      mWeaponSlot = 0;

   navigate();
}


// Show previous panel
void UIXtankHelper::navigateBackward(bool changePhase)
{
   mTransitionFromPhase = mPhase;   // Start transition animation
   mTransitioningForward = false;
   if(changePhase)
      mPhase = prevEnum(mPhase);

   // When entering weapon phases from non-weapon phases, start at first empty slot
   if(mPhase == Phase::WEAPONS && mTransitionFromPhase != Phase::WEAPONS)
      mWeaponSlot = MIN(mDesignInProgress.slotsInUse(), WEAPON_SLOTS - 1);

   navigate();
}


void UIXtankHelper::navigate()
{
   mTransitionTimer.reset(250);	   // 250ms transition

   S32 newWidth = widthForPhase(mPhase);
   setExpectedWidth_MidTransition(newWidth);
   resetScrollTimer();
   mHighlightedIndex = getHighlightedIndexForPhase();
}


S32 UIXtankHelper::widthForPhase(Phase phase) const
{
   if(phase == Phase::BODY)        return getTotalDisplayWidth(mBodyButtonsWidth, mBodyItemsDisplayWidth);
   if(phase == Phase::ENGINE)      return getTotalDisplayWidth(mEngineButtonsWidth, mEngineItemsDisplayWidth);
   if(phase == Phase::TREADS)      return getTotalDisplayWidth(mTreadButtonsWidth, mTreadItemsDisplayWidth);
   if(phase == Phase::ARMOR)       return getTotalDisplayWidth(mArmorButtonsWidth, mArmorItemsDisplayWidth);
   if(phase == Phase::ARMOR_SIDES) return getTotalDisplayWidth(mArmorSidesButtonsWidth, mArmorSidesItemsDisplayWidth);
   if(phase == Phase::SUSPENSION)  return getTotalDisplayWidth(mSuspensionButtonsWidth, mSuspensionItemsDisplayWidth);
   if(phase == Phase::BUMPERS)     return getTotalDisplayWidth(mBumperButtonsWidth, mBumperItemsDisplayWidth);
   if(phase == Phase::SPECIALS)    return getTotalDisplayWidth(mSpecialsButtonsWidth, mSpecialsItemsDisplayWidth);
   if(phase == Phase::HEATSINK)    return getTotalDisplayWidth(mHeatSinkButtonsWidth, mHeatSinkItemsDisplayWidth);
   if(phase == Phase::WEAPONS)     return getTotalDisplayWidth(mWeaponButtonsWidth, mWeaponItemsDisplayWidth);
   TNLAssert(false, "Unknown phase");
   return 100;	 // Suppress warning; unreachable
}


// Set mHighlightedIndex to reflect the current value in mDesignInProgress for
// the given phase, so when the player navigates back they see their own choice
// already highlighted.
S32 UIXtankHelper::getHighlightedIndexForPhase() const
{
   switch(mPhase)
   {
   case Phase::BODY:
      return ((S32)mDesignInProgress.body >= 0) ? (S32)mDesignInProgress.body : 0;
   case Phase::ENGINE:
      return (S32)mDesignInProgress.engine;
   case Phase::TREADS:
      return (S32)mDesignInProgress.tread;
   case Phase::ARMOR:
      return (S32)mDesignInProgress.armor;
   case Phase::ARMOR_SIDES:
      return 0;	   // Default to Front (index 0) when entering; preserve if already valid.
   case Phase::SUSPENSION:
      return (S32)mDesignInProgress.suspension;
   case Phase::BUMPERS:
      return (S32)mDesignInProgress.bumper;
   case Phase::SPECIALS:

      // Position cursor on the first active special, or 0 if none active.
      //return 0;
      for(S32 i = 0; i < XtankSpecialCount; i++)
         if(hasSpecial(mDesignInProgress.specials, (XtankSpecial)i))
            return i;

      return 0;

   case Phase::HEATSINK:
      return mDesignInProgress.heatSinks;

   case Phase::WEAPONS:
      if(mDesignInProgress.weapons[mWeaponSlot] == XtankWeapon::NONE)
         return 0;
      else
         return (S32)mDesignInProgress.weapons[mWeaponSlot] + 1;		// +1 to accomodate 0th None slot
   }
   TNLAssert(false, "Unhandled phase");
   return 100;	// Suppress warning; unreachable
}


const Vector<OverlayMenuItem> *UIXtankHelper::getItemsForPhase(Phase phase) const
{
   switch(phase)
   {
   case Phase::BODY:        return &mBodyItems;
   case Phase::ENGINE:      return &mEngineItems;
   case Phase::TREADS:      return &mTreadItems;
   case Phase::ARMOR:       return &mArmorItems;
   case Phase::ARMOR_SIDES: return &mArmorSidesItems;
   case Phase::SUSPENSION:  return &mSuspensionItems;
   case Phase::BUMPERS:     return &mBumperItems;
   case Phase::SPECIALS:    return &mSpecialsItems;
   case Phase::HEATSINK:    return nullptr;
   case Phase::WEAPONS:     return &mWeaponItems;
   }

   TNLAssert(false, "Unknown phase");
   return nullptr;	   // Suppress warning; unreachable
}


// Generic component table renderer for components in the vehicle designer.
// Returns the y-position just below the last rendered table row.
S32 ComponentInfo::render(S32 left, S32 top, S32 fontSize, S32 rowGap, S32 highlightedRow)
{
   const S32 columnCount = getColCount();
   const S32 columnSpacing = 6;

   computeColWidths(fontSize);

   Renderer &renderer = Renderer::get();
   S32 y = top;
   S32 x = left;

   renderer.setColor(TABLE_HEADER_COLOR);

   // Compute the tallest header (multiline support) for bottom-justification.
   S32 maxHeaderLines = 1;
   for(S32 c = 0; c < columnCount; c++)
   {
      const char *hdr = getColumns()[c].header ? getColumns()[c].header : "";
      S32 lines = 1;

      // Scan for \n chars
      for(const char *p = hdr; *p; p++)
         if(*p == '\n')
            lines++;

      if(lines > maxHeaderLines)
         maxHeaderLines = lines;
   }

   for(S32 c = 0; c < columnCount; c++)
   {
      TableColumn col = getColumns()[c];
      const char *hdr = col.header ? col.header : "";

      S32 thisLines = 1;
      for(const char *p = hdr; *p; p++)
         if(*p == '\n') thisLines++;

      // Bottom-justify: start so the last line aligns with maxHeaderLines.
      S32 lineY = y + (maxHeaderLines - thisLines) * (fontSize + 2);
      const char *seg = hdr;
      while(true)
      {
         const char *nl = strchr(seg, '\n');
         S32 segLen = nl ? (S32)(nl - seg) : (S32)strlen(seg);
         char segBuf[64];
         segLen = MIN(segLen, (S32)sizeof(segBuf) - 1);
         strncpy(segBuf, seg, segLen);
         segBuf[segLen] = '\0';

         if(col.headerAlignment == ALIGN_RIGHT)
            drawStringr(x + mColTotalWidths[c] + col.headerNudge, lineY, fontSize, segBuf);
         else if(col.headerAlignment == ALIGN_CENTER)
            drawStringc(x + mColTotalWidths[c] / 2 + col.headerNudge, lineY + fontSize, fontSize, segBuf);
         else
            drawString(x + col.headerNudge, lineY, fontSize, segBuf);

         lineY += fontSize + 2;
         if(!nl) break;
         seg = nl + 1;
      }
      x += mColTotalWidths[c] + columnSpacing;
   }
   y += rowGap + (maxHeaderLines - 1) * (fontSize + 2);

   for(S32 r = 0; r < getRowCount(); r++)
   {
      x = left;
      for(S32 c = 0; c < columnCount; c++)
      {
         if(r == highlightedRow)
            renderer.setColor(SELECTED_COLOR);
         else if(c == 1)
            renderer.setColor(UNSELECTED_ITEM_COLOR_NAME);
         else
            renderer.setColor(UNSELECTED_ITEM_COLOR_DATA);

         const char *value = getRows()[r].cells[c] ? getRows()[r].cells[c] : "";
         ColAlignmemt align = getColumns()[c].dataAlignment;
         //if(!strcmp(value, "--"))
            //align = ALIGN_RIGHT;

         if(align == ALIGN_RIGHT)
         {
            S32 adj = MAX((mColHeaderWidths[c] - mColDataWidths[c]) / 2, 0);
            drawStringr(x + mColTotalWidths[c] - adj, y, fontSize, value);
         }
         else if(align == ALIGN_CENTER)
            drawStringc(x + mColTotalWidths[c] / 2, y + fontSize, fontSize, value);
         else
            drawString(x, y, fontSize, value);

         x += mColTotalWidths[c] + columnSpacing;
      }
      y += rowGap;
   }

   return y;
}


static S32 doDrawTwoColors(S32 x, S32 y, S32 size, const Color &color1, const char *string1, const Color &color2)
{
   Renderer &r = Renderer::get();

   r.setColor(color1);
   x += drawStringAndGetWidth(x, y, size, string1);
   x += getStringWidth(size, " ");
   r.setColor(color2);
   return x;
}


static void drawTwoColors(S32 x, S32 y, S32 size, const Color &color1, const char *string1, const Color &color2, S32 val)
{
   x = doDrawTwoColors(x, y, size, color1, string1, color2);
   drawString(x, y, size, cs(comma(val)));
}


static void drawTwoColors(S32 x, S32 y, S32 size, const Color &color1, const char *string1, const Color &color2, const char *format, const char *string2)
{
   x = doDrawTwoColors(x, y, size, color1, string1, color2);
   drawStringf(x, y, size, format, string2);
}

static const S32 STAT_SZ = 12;
static const S32 GAP = STAT_SZ + 5;


// Render the right half of the armor allocation card
void UIXtankHelper::renderArmorStats(S32 x, S32 y, S32 size, F32 alpha) const
{
   Renderer &r = Renderer::get();

   S32 armorIdx = (S32)mDesignInProgress.armor;
   S32 bodyIdx = (S32)mDesignInProgress.body;
   S32 bodySize = xTankBodyStats[bodyIdx].size;
   const XtankArmorInfo &ai = xtankArmorInfos[armorIdx];

   S32 total = 0;
   for(S32 i = 0; i < VehicleSidesCount; i++)
      total += mDesignInProgress.armorSides[i];

   S32 totalWeight = (S32)(total * ai.weight * bodySize);
   S32 totalCost = (S32)(total * ai.cost * bodySize);
   S32 totalSpace = (S32)(total * ai.space * bodySize);

   Color col1 = Color(0, alpha, alpha);
   Color col2 = Color(alpha, alpha, alpha);

   drawTwoColors(x, y, size, col1, "Total Armor:", col2, total == 1 ? "%s / unit" : "%s / units", cs(itos(total)));   y += GAP;
   drawTwoColors(x, y, size, col1, "Type:", col2, "%s", mDesignInProgress.getArmorName());                            y += GAP;
   drawTwoColors(x, y, size, col1, "Weight:", col2, "%s / unit", cs(itos(ai.weight)));                                y += GAP;
   drawTwoColors(x, y, size, col1, "Space:", col2, "%s / unit", cs(itos(ai.space)));                                  y += GAP;
   drawTwoColors(x, y, size, col1, "Cost:", col2, "%s / unit", cs(itos(ai.cost)));                                    y += GAP;
   y += size;

   drawTwoColors(x, y, size, col1, "Vehicle Size:", col2, "%s", cs(itos(mDesignInProgress.getBodySize())));	          y += GAP;
   y += size;

   drawTwoColors(x, y, size, col1, "Total Armor Weight:", col2, totalWeight);       y += GAP;
   drawTwoColors(x, y, size, col1, "Space:", col2, totalSpace);                     y += GAP;
   drawTwoColors(x, y, size, col1, "Cost:", col2, totalCost);					    y += GAP;
}


void UIXtankHelper::renderSpecialsStats(S32 left, S32 y, F32 alpha) const
{
   Renderer &r = Renderer::get();

   S32 idx = mHighlightedIndex;

   if(idx < 0 || idx >= XtankSpecialCount) idx = 0;
   const XtankSpecialInfo &si = xtankSpecialInfos[idx];
   bool active = hasSpecial(mDesignInProgress.specials, (XtankSpecial)idx);

   r.setColor(active ? Colors::green : Colors::overlayMenuUnselectedItemColor);
   drawString(left, y, STAT_SZ, active ? "ACTIVE" : "inactive");    y += GAP;

   r.setColor(Color(0.0f, alpha, alpha));
   drawStringf(left, y, STAT_SZ, "Space:  %d", si.space);           y += GAP;
   drawStringf(left, y, STAT_SZ, "Weight: %d", si.weight);          y += GAP;
   drawStringf(left, y, STAT_SZ, "Cost:   %s", cs(comma(si.cost))); y += GAP;
}



// Render one panel.
// centerFraction: 0.0 = fully background (adjacent), 1.0 = fully foreground (center).
void UIXtankHelper::renderCard(S32 left, S32 top, S32 right, S32 bot, Phase phase, F32 cf)
{
   if(phase == Phase::NONE)
      return;

   static const S32 CORNER = 10;
   static const S32 TITLE_SZ = 16;   // fixed title font size, same on ALL cards
   // Approximate available list height on the center card (matches SLOT C geometry: bot=595, top=135).
   static const S32 CTR_LIST_AVAIL = 595 - 6 - (100 + 8 + TITLE_SZ + 4 + 6);  // 595-6-134 = 455

   F32 alpha = (cf - 0.55f) / 0.45f;


   // Phase title — one entry per phase.
   static const char *sPhaseTitles[] =
   {
      "Body",              // 0
      "Engine",            // 1
      "Treads",            // 2
      "Armor Type",        // 3
      "Armor Allocation",  // 4
      "Suspension",        // 5
      "Bumpers",           // 6
      "Specials",          // 7
      "Heat Sinks",        // 8
      "Weapons",           // 9		<== Not used; we call renderWeaponPanelTitle() instead
      "Mounts",            // 10
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
   const S32 w = right - left;
   const S32 STATS_DIV = left + w * 54 / 100;


   // Custom title for Weapons, standard title for everyone else
   r.setColor(isCenter ? Colors::white : Color(grayLevel, grayLevel, grayLevel));
   S32 titlex = cx;
   S32 titley = top + 8;

   if(phase == Phase::WEAPONS)
      renderWeaponPanelTitle(titlex, titley, right, TITLE_SZ, isCenter);
   else
      drawCenteredString(titlex, titley, TITLE_SZ, sPhaseTitles[(S32)phase]);

   // Divider
   r.setColor(Color(0.20f + cf * 0.20f, 0.20f + cf * 0.20f, 0.20f + cf * 0.20f));
   const S32 divY = top + 8 + TITLE_SZ + 4;

   const S32 titleUnderlineMargin = 12;
   drawHorizLine(left + titleUnderlineMargin, right - titleUnderlineMargin, divY);	// Border line

   S32 contentTop = top + 34;	 // y-coord of the top of the card content, below the title and the border line

   const Vector<OverlayMenuItem> *items = getItemsForPhase(phase);

   // Special case:
   if(phase == Phase::HEATSINK)
      renderHeatSinkPanel(contentTop, left, alpha, STATS_DIV);
   else
      renderItemTable(items, contentTop, left, isCenter, phase, mHighlightedIndex, grayLevel);


   // Stats column — center card only, fades in; suppressed when a table fills the card.
   if(cf > 0.55f)	   // Only true for Armor Allocation and specials at the moment
   {
      const S32 STATS_DIV = left + w * 54 / 100;


      if(phase == Phase::ARMOR_SIDES)
         renderArmorStats(STATS_DIV, contentTop, STAT_SZ, alpha);
      else if(phase == Phase::SPECIALS)
         renderSpecialsStats(STATS_DIV, contentTop, alpha);
   }


   // Navigation hint — center card only
   if(isCenter)
   {
      r.setColor(Colors::gray60);
      if(phase == Phase::ARMOR_SIDES)
         drawCenteredString(cx, bot - 14, 10, "Up/Dn=side  +/-=adjust  Tab=next  Esc=cancel");
      else
         drawCenteredString(cx, bot - 14, 10, "Tab  Enter=confirm  Esc=cancel");
   }
}


// Special heatsink panel
void UIXtankHelper::renderHeatSinkPanel(S32 top, S32 left, F32 alpha, S32 colTwoX)
{
   Renderer &r = Renderer::get();

   const S32 margin = 9;

   S32 n = mDesignInProgress.heatSinks;

   S32 modifier = mDesignInProgress.heatDissipation();

   S32 x = left + margin;
   S32 y = top;

   r.setColor(Colors::overlayMenuUnselectedItemColor, alpha);
   x += drawStringAndGetWidth(x, y, 16, "Heat Sinks: ");
   r.setColor(SELECTED_COLOR, alpha);
   drawString(x, y, 16, cs(itos(mDesignInProgress.heatSinks)));

   x = colTwoX;

   Color col1 = Color(0, alpha, alpha);
   Color col2 = Color(alpha, alpha, alpha);

   drawTwoColors(x, y, STAT_SZ, col1, "Heat Dissipation:", col2, modifier);                  y += GAP;
   drawTwoColors(x, y, STAT_SZ, col1, "Weight:",           col2, heatSinkStat.weight * n);   y += GAP;
   drawTwoColors(x, y, STAT_SZ, col1, "Cost:",             col2, heatSinkStat.cost * n);     y += GAP;
}


// Draw a table of items
void UIXtankHelper::renderItemTable(const Vector<OverlayMenuItem> *items, S32 top, S32 left, bool isActive, Phase phase, S32 highlightedIndex, F32 grayLevel)
{
   if(!items || items->size() == 0)
      return;

   static const S32 CTR_ROW_GAP = 16;


   const S32 count = items->size();

   Renderer &r = Renderer::get();


   // Center card: fixed CTR_ROW_GAP so all menus look the same.
   // Background card: scale the gap proportionally to its available height.

   const S32 rowGap = 16;
   static const S32 ITEM_SZ = 13;   // fixed item font size, same on ALL cards
   const S32 drawnSz = MIN(ITEM_SZ, MAX(7, rowGap - 3));

   // Selection bar
   const S32 BAR_HEIGHT = 18;
   const S32 BAR_WIDTH = 3;
   const S32 BAR_MARGIN_R = 6;      // Space between selection bar and key code
   const S32 BAR_MARGIN_L = 3;      // Space between edge of box and selection bar
   const S32 KEY_COL_WIDTH = 10;

   // Column positions
   const S32 KEY_X = left + BAR_WIDTH + BAR_MARGIN_R;
   const S32 NAME_X = isActive ? (left + 30) : (left + 6);

   ComponentInfo *tableInfo = getCurrentComponentInfo(isActive);

   if(tableInfo)
      tableInfo->render(left + 8, top, drawnSz, rowGap, highlightedIndex);
   else
   {
      // "[X] " + longest special name fits comfortably in 64 chars.
      static const S32 SPECIAL_LABEL_BUF_LEN = 64;
      static const S32 ARMOR_SIDE_COUNT = 6;
      char sSpecialNameBuf[XtankSpecialCount][SPECIAL_LABEL_BUF_LEN];

      for(S32 i = 0; i < count; i++)
      {
         S32 y = top + i * rowGap;

         const char *name = (*items)[i].name;
         const char *name2 = (*items)[i].name;
         if(!name)
            name = "";

         // For specials phase, prefix with toggle state indicator
         if(phase == Phase::SPECIALS && i < XtankSpecialCount)
         {
            bool active = hasSpecial(mDesignInProgress.specials, (XtankSpecial)i);
            dSprintf(sSpecialNameBuf[i], sizeof(sSpecialNameBuf[i]), "%s %s",
               active ? "[X]" : "[ ]", name);
            name = sSpecialNameBuf[i];
         }

         bool sel = isActive && (i == mHighlightedIndex);

         if(isActive)
         {
            if(sel)
            {
               // Yellow bar: vertically centered on the text with 3px padding each side
               S32 barTop = y + drawnSz / 2 - (drawnSz / 2 + 3) + 2;
               drawFilledRect(left + BAR_MARGIN_L, barTop, left + BAR_MARGIN_L + BAR_WIDTH, barTop + BAR_HEIGHT, SELECTED_COLOR);
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
   }
}


ComponentInfo *UIXtankHelper::getCurrentComponentInfo(bool isCenter)
{
   // On the center card, use a ComponentInfo table renderer for phases that have one.
   // Background cards always fall through to the generic item-list renderer.
   if(isCenter)
   {
      switch(mPhase)
      {
         case Phase::BODY:        return &mBodyInfo;
         case Phase::ARMOR:       return &mArmorInfo;
         case Phase::ENGINE:      return &mEngineInfo;
         case Phase::TREADS:      return &mTreadInfo;
         case Phase::SUSPENSION:  return &mSuspensionInfo;
         case Phase::BUMPERS:     return &mBumperInfo;
         case Phase::WEAPONS:     return &mWeaponInfo;
         //case Phase::HEATSINK:    return &mHeatSinkInfo;
         case Phase::ARMOR_SIDES: return &mArmorAllocationInfo;
      }
   }

   return nullptr;
}


void UIXtankHelper::renderWeaponPanelTitle(S32 titlex, S32 titley, S32 right, S32 size, bool isCenter)
{
   Renderer &r = Renderer::get();

   static char slotBuf[128];
   XtankWeapon weapon = mDesignInProgress.weapons[mWeaponSlot];
   XtankMountLocation mount = mDesignInProgress.weaponMounts[mWeaponSlot];

   // On the active center weapons card, show the live highlighted selection
   // (same preview behavior as the right-side vehicle panel) before commit.
   //if(isCenter)
   {
      if(mHighlightedIndex <= 0)
      {
         weapon = XtankWeapon::NONE;
         mount = XtankMountLocation::NONE;
      }
      else if(mHighlightedIndex < mWeaponItems.size())
      {
         weapon = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;
         mount = mDesignInProgress.firstValidMount(mDesignInProgress.body, weapon, mount);
      }
   }

   const char *mode = "Weapon";
   const char *weaponName = "No Weapon";

   if(weapon != XtankWeapon::NONE)
      weaponName = xtankWeaponInfos[(S32)weapon].name;

   dSprintf(slotBuf, sizeof(slotBuf), "%s #%d / %d : %s (%s)", mode, mWeaponSlot + 1, WEAPON_SLOTS, weaponName, getMountLabel(mount));

   drawCenteredString(titlex, titley, size, slotBuf);
}


void UIXtankHelper::renderFloatingMenus()
{
   // -------------------------------------------------------------------------
   // Slot geometry table.  5 slots: L2(off) L1(adj) C(center) R1(adj) R2(off).
   // "tuck" factor: adjacent cards start a few pixels lower and smaller,
   // giving the impression they are physically behind the center card.
   //
   //   Slot  cx    half_w  top   bot   centerFraction
   //   L2    -80     55    180   520   0.0   (off-screen, invisible)
   //   L1     90     80    155   545   0.0   (tucked behind left edge of center)
   //   C     375    190    100   595   1.0   (full foreground)
   //   R1    660     80    155   545   0.0   (tucked behind right edge of center)
   //   R2    910     55    180   520   0.0   (off-screen, invisible)
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
      { -80,  55,  180, 520,  0.0f },  // 0 = L2  off-screen
      {  90,  80,  155, 545,  0.0f },  // 1 = L1  adjacent left  (tucked)
      { 375, 190,  100, 595,  1.0f },  // 2 = C   center
      { 660,  80,  155, 545,  0.0f },  // 3 = R1  adjacent right (tucked)
      { 910,  55,  180, 520,  0.0f },  // 4 = R2  off-screen
   };

   const S32 totalPhases = (S32)Phase::COUNT;

   // Smoothstepped elapsed fraction: 0 at start of transition, 1 at end.
   F32 t = 0.0f;
   if(mTransitionTimer.getCurrent() > 0)
   {
      F32 e = 1.0f - mTransitionTimer.getFraction();
      t = e * e * (3.0f - 2.0f * e);
   }


   static const S32 CARDS_TO_DISPLAY = 5;

   Phase phaseAtSlot[CARDS_TO_DISPLAY];
   S32 weaponSlotAtSlot[CARDS_TO_DISPLAY];

   computeCarouselSlots(mPhase, mWeaponSlot, mDesignInProgress.slotsInUse(), phaseAtSlot, weaponSlotAtSlot, CARDS_TO_DISPLAY);


   struct CardAnim { Phase phase; S32 weaponSide; S32 fromSlot; S32 toSlot; };
   CardAnim cards[CARDS_TO_DISPLAY];

   if(mTransitionTimer.getCurrent() > 0)
   {
      if(mTransitioningForward)
         for(S32 i = 0; i < CARDS_TO_DISPLAY; i++)
            cards[i] = { phaseAtSlot[i], weaponSlotAtSlot[i], MIN(i + 1, CARDS_TO_DISPLAY - 1), i };
      else
         for(S32 i = 0; i < CARDS_TO_DISPLAY; i++)
            cards[i] = { phaseAtSlot[i], weaponSlotAtSlot[i], MAX(i - 1, 0), i };
   }
   else
      for(S32 i = 0; i < CARDS_TO_DISPLAY; i++)
         cards[i] = { phaseAtSlot[i], weaponSlotAtSlot[i], i, i };

   FontManager::pushFontContext(HelperMenuContext);


   // Draw back-to-front so center card is always on top
   static const S32 drawOrder[CARDS_TO_DISPLAY] = { 0, 4, 1, 3, 2 };

   for(S32 drawIndex = 0; drawIndex < CARDS_TO_DISPLAY; drawIndex++)
   {
      S32 currentDrawIndex = drawOrder[drawIndex];

      const CardAnim &ca = cards[currentDrawIndex];
      const SlotGeom &s0 = SLOTS[ca.fromSlot];	   // Drawing coordinates
      const SlotGeom &s1 = SLOTS[ca.toSlot];

      // t is transition time, so these coordinates change every iteration until animation is complete
      S32 cx = s0.cx + (S32)((s1.cx - s0.cx) * t);
      S32 half_w = s0.half_w + (S32)((s1.half_w - s0.half_w) * t);
      S32 top = s0.top + (S32)((s1.top - s0.top) * t) - 80;
      S32 bot = s0.bot + (S32)((s1.bot - s0.bot) * t) - 80;
      F32 cf = s0.cf + (s1.cf - s0.cf) * t;

      // Skip fully invisible cards
      if(cf < 0.02f && half_w < 70) continue;
      if(cx + half_w < 0 || cx - half_w > 800) continue;

      renderCard(cx - half_w, top, cx + half_w, bot, ca.phase, cf);
   }

   FontManager::popFontContext();
}


// Build the 5 cards shown this frame.
//
// The navigable sequence is linear (no wrapping):
//   virtual pos 0 .. WEAPONS-1             => one card per non-weapon phase
//   virtual pos WEAPONS .. +slotsInUse-1   => one card per weapon sub-slot
//
// Display slot 2 = center (current), 1 = one back, 0 = two back,
// slot 3 = one ahead, 4 = two ahead.  Clamped so we never show a
// position before BODY or past the last weapon slot.
void UIXtankHelper::computeCarouselSlots(Phase currentPhase, S32 currentWeaponSlot, S32 slotsInUse,
                                         Phase phaseAtSlot[], S32 weaponSlotAtSlot[], // <== Populate these arrays
                                         S32 cardsToFill)
{
   // Number of actual panels considering that there may be multiple weapons panels, 
   // +1 for the extra blank panel at the end, unless that causes us to exceed WEAPON_SLOTS
   S32 totalPanels = (S32)Phase::WEAPONS + MIN(slotsInUse + 1, WEAPON_SLOTS);	  

   // Virtual position of the currently active panel in the list of totalPanels
   S32 currentPanelIndex = (currentPhase == Phase::WEAPONS) ? (S32)Phase::WEAPONS + currentWeaponSlot : (S32)currentPhase;

   for(S32 i = 0; i < cardsToFill; i++)
   {
      S32 desiredPanel = currentPanelIndex + (i - 2);	 // Panel we'd want if panels were unbounded
      S32 panel = CLAMP(desiredPanel, 0, totalPanels);

      if(panel < (S32)Phase::WEAPONS)
      {
         if(desiredPanel < 0)
            phaseAtSlot[i] = Phase::NONE;	// <== Phase::NONE means this slot is blank (used for padding when we don't have enough panels to fill all slots)
         else
         {
            phaseAtSlot[i] = (Phase)panel;	// <== This is the phase of panel i where the midpoint of cards is the current panel
            weaponSlotAtSlot[i] = -1;		// <== If the phase is Phase::WEAPONS, this is the weapons slot associated with that panel
         }
      }
      else
      {
         if(desiredPanel > panel)
            phaseAtSlot[i] = Phase::NONE;
         else
         {
            phaseAtSlot[i] = Phase::WEAPONS;
            weaponSlotAtSlot[i] = panel - (S32)Phase::WEAPONS;
         }
      }
   }
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
S32 UIXtankHelper::currentPhaseItemCount()
{
   if(mPhase == Phase::ARMOR_SIDES)
      return VehicleSidesCount;
   ComponentInfo *componentInfo = getCurrentComponentInfo(true);
   if(componentInfo)
      return componentInfo->getRowCount();
   else
      return -1;
}


// Draw a row of small circles indicating the current carousel position.
//// Since phase navigation wraps, both arrows are always active.
//void UIXtankHelper::renderCarouselDots(S32 cx, S32 y) const
//{
//   S32 totalPhases = Phase::COUNT;
//
//   static const F32 DOT_R      = 4.0f;
//   static const F32 DOT_R_SM   = 3.0f;
//   static const F32 DOT_STEP   = 14.0f;
//   static const S32 ARROW_GAP  = 12;
//   static const S32 ARROW_SZ   = 11;
//
//   // Centre the dots row at cx.
//   F32 rowWidth   = F32(totalPhases - 1) * DOT_STEP;
//   F32 dotsLeft   = F32(cx) - rowWidth * 0.5f;
//
//   Renderer &r = Renderer::get();
//   static const S32 SEG = 12;  // polygon segments for each circle
//
//   for(S32 i = 0; i < totalPhases; i++)
//   {
//      F32 dx  = dotsLeft + F32(i) * DOT_STEP;
//      F32 rad = (i == mPhase) ? DOT_R : DOT_R_SM;
//      bool filled = (i == mPhase);
//
//      if(i == mPhase)
//         r.setColor(Colors::white);
//      else if(i < mPhase)
//         r.setColor(Colors::gray67);
//      else
//         r.setColor(Colors::gray40);
//
//      if(filled)
//      {
//         F32 verts[2 + (SEG + 1) * 2];
//         verts[0] = dx;
//         verts[1] = F32(y);
//         for(S32 s = 0; s <= SEG; s++)
//         {
//            F32 a = FloatPi * 2.0f * F32(s % SEG) / F32(SEG);
//            verts[2 + s * 2]     = dx + rad * cosf(a);
//            verts[2 + s * 2 + 1] = F32(y) + rad * sinf(a);
//         }
//         r.renderVertexArray(verts, 2 + SEG, RenderType::TriangleFan);
//      }
//      else
//      {
//         F32 verts[SEG * 2];
//         for(S32 s = 0; s < SEG; s++)
//         {
//            F32 a = FloatPi * 2.0f * F32(s) / F32(SEG);
//            verts[s * 2]     = dx + rad * cosf(a);
//            verts[s * 2 + 1] = F32(y) + rad * sinf(a);
//         }
//         r.renderVertexArray(verts, SEG, RenderType::LineLoop);
//      }
//   }
//
//   // Wrap navigation is always available both directions.
//   S32 arrowY = y - ARROW_SZ / 2;
//   S32 leftArrowX = S32(dotsLeft) - ARROW_GAP - ARROW_SZ;
//   r.setColor(Colors::gray67);
//   drawCenteredString(leftArrowX, arrowY, ARROW_SZ, "<");
//
//   S32 rightArrowX = S32(dotsLeft + rowWidth) + ARROW_GAP;
//   r.setColor(Colors::gray67);
//   drawCenteredString(rightArrowX, arrowY, ARROW_SZ, ">");
//}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// Lazy initializer
void ComponentInfo::computeColWidths(S32 fontSize)
{
   if(mComputed)
      return;

   fillRows();

   for(S32 c = 0; c < getColCount(); c++)
   {
      // Header width: handle multiline headers by taking the widest segment.
      const char *hdr = getColumns()[c].header ? getColumns()[c].header : "";
      S32 hdrW = 0;
      const char *seg = hdr;
      while(true)
      {
         const char *nl = strchr(seg, '\n');
         S32 segLen = nl ? (S32)(nl - seg) : (S32)strlen(seg);
         char segBuf[64];
         segLen = MIN(segLen, (S32)sizeof(segBuf) - 1);
         strncpy(segBuf, seg, segLen);
         segBuf[segLen] = '\0';
         hdrW = MAX(hdrW, getStringWidth(fontSize, segBuf));
         if(!nl) break;
         seg = nl + 1;
      }
      mColHeaderWidths[c] = hdrW;

      S32 w = 0;
      for(S32 r = 0; r < getRowCount(); r++)
      {
         const char *value = getRows()[r].cells[c] ? getRows()[r].cells[c] : "";
         w = MAX(w, getStringWidth(fontSize, value));
      }
      mColDataWidths[c] = w;
      mColTotalWidths[c] = MAX(w, hdrW);
   }
   mComputed = true;
}


static const char *getArmorClassName(S32 defenseValue)
{
   switch(defenseValue)
   {
   case 0: return "Light";
   case 1: return "Standard";
   case 2: return "Heavy";
   case 3: return "Super";
   default: return "?";
   }
}


void ArmorInfo::fillRows()
{
   static char weightBuf[XtankArmorCount][COL_LEN];
   static char spaceBuf[XtankArmorCount][COL_LEN];
   static char costBuf[XtankArmorCount][COL_LEN];

   for(S32 i = 0; i < XtankArmorCount; i++)
   {
      const XtankArmorInfo &ai = xtankArmorInfos[i];
      dSprintf(weightBuf[i], COL_LEN, "%d", ai.weight);
      dSprintf(spaceBuf[i], COL_LEN, "%d", ai.space);
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(ai.cost)));

      rows[i].cells[0] = InputCodeManager::inputCodeToString(hotKeys[i]);
      rows[i].cells[1] = ai.name;
      rows[i].cells[2] = getArmorClassName(ai.defense);
      rows[i].cells[3] = weightBuf[i];
      rows[i].cells[4] = spaceBuf[i];
      rows[i].cells[5] = costBuf[i];
      rows[i].highlighted = false;
   }
}


void BodyInfo::fillRows()
{
   static char weightBuf[VehicleBodyCount][COL_LEN];
   static char weightLimitBuf[VehicleBodyCount][COL_LEN];
   static char spaceBuf[VehicleBodyCount][COL_LEN];
   static char dragBuf[VehicleBodyCount][COL_LEN];
   static char handlingBuf[VehicleBodyCount][COL_LEN];
   static char turretsBuf[VehicleBodyCount][COL_LEN];
   static char costBuf[VehicleBodyCount][COL_LEN];

   for(S32 i = 0; i < VehicleBodyCount; i++)
   {
      const XtankBodyInfo2 &bi = xTankBodyStats[i];
      dSprintf(weightBuf[i], COL_LEN, "%s", cs(comma(bi.weight)));
      dSprintf(weightLimitBuf[i], COL_LEN, "%s", cs(comma(bi.weightLimit)));
      dSprintf(spaceBuf[i], COL_LEN, "%s", cs(comma(bi.space)));
      dSprintf(dragBuf[i], COL_LEN, "%.2f", bi.drag);
      dSprintf(handlingBuf[i], COL_LEN, "%d", bi.handling);
      dSprintf(turretsBuf[i], COL_LEN, "%d", bi.turrets);
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(bi.cost)));

      rows[i].cells[0] = InputCodeManager::inputCodeToString(hotKeys[i]);
      rows[i].cells[1] = bi.name;
      rows[i].cells[2] = weightBuf[i];
      rows[i].cells[3] = weightLimitBuf[i];
      rows[i].cells[4] = spaceBuf[i];
      rows[i].cells[5] = dragBuf[i];
      rows[i].cells[6] = handlingBuf[i];
      rows[i].cells[7] = turretsBuf[i];
      rows[i].cells[8] = costBuf[i];
      rows[i].highlighted = false;
   }
}


void EngineInfo::fillRows()
{
   //static char spdBuf[XtankEngineCount][COL_LEN];
   //static char accBuf[XtankEngineCount][COL_LEN];
   static char pwrBuf[XtankEngineCount][COL_LEN];
   static char wtBuf[XtankEngineCount][COL_LEN];
   static char spaceBuf[XtankEngineCount][COL_LEN];
   static char fuelCostBuf[XtankEngineCount][COL_LEN];
   static char fuelCapBuf[XtankEngineCount][COL_LEN];

   static char costBuf[XtankEngineCount][COL_LEN];

   for(S32 i = 0; i < XtankEngineCount; i++)
   {
      const XtankEngineInfo &ei = xtankEngineInfos[i];
      //dSprintf(spdBuf[i], COL_LEN, "%.2f", (S32)((ei.speedMult - 1.0f) * 100.0f + 0.5f));  // +0.5f rounds to nearest int
      //dSprintf(accBuf[i], COL_LEN, "%.2f", (S32)((ei.accelMult - 1.0f) * 100.0f + 0.5f));
      dSprintf(pwrBuf[i], COL_LEN, "%s", cs(comma(ei.power)));
      dSprintf(wtBuf[i], COL_LEN, "%s", cs(comma(ei.weight)));
      dSprintf(spaceBuf[i], COL_LEN, "%s", cs(comma(ei.space)));
      dSprintf(fuelCostBuf[i], COL_LEN, "%d", ei.fuel);
      dSprintf(fuelCapBuf[i], COL_LEN, "%s", cs(comma(ei.fcap)));
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(ei.cost)));

      rows[i].cells[0] = InputCodeManager::inputCodeToString(getKeyForIndex(i));
      rows[i].cells[1] = ei.name;
      //rows[i].cells[2] = spdBuf[i];
      //rows[i].cells[3] = accBuf[i];
      rows[i].cells[2] = pwrBuf[i];
      rows[i].cells[3] = wtBuf[i];
      rows[i].cells[4] = spaceBuf[i];
      rows[i].cells[5] = fuelCostBuf[i];
      rows[i].cells[6] = fuelCapBuf[i];
      rows[i].cells[7] = costBuf[i];
      rows[i].highlighted = false;
   }
}


void TreadInfo::fillRows()
{
   static char frictBuf[XtankTreadCount][COL_LEN];
   static char costBuf[XtankTreadCount][COL_LEN];

   for(S32 i = 0; i < XtankTreadCount; i++)
   {
      const XtankTreadInfo &ti = xtankTreadInfos[i];
      dSprintf(frictBuf[i], COL_LEN, "%.2f", ti.friction);
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(ti.cost)));

      rows[i].cells[0] = InputCodeManager::inputCodeToString(getKeyForIndex(i));
      rows[i].cells[1] = ti.name;
      rows[i].cells[2] = frictBuf[i];
      rows[i].cells[3] = costBuf[i];
      rows[i].highlighted = false;
   }
}


void SuspensionInfo::fillRows()
{
   static char handlingBuf[XtankSuspensionCount][COL_LEN];
   static char costBuf[XtankSuspensionCount][COL_LEN];

   for(S32 i = 0; i < XtankSuspensionCount; i++)
   {
      const SuspensionStat &si = suspensionStat[i];
      dSprintf(handlingBuf[i], COL_LEN, "%+d", (S32)si.friction);
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(si.cost)));

      rows[i].cells[0] = InputCodeManager::inputCodeToString(getKeyForIndex(i));
      rows[i].cells[1] = si.name;
      rows[i].cells[2] = handlingBuf[i];
      rows[i].cells[3] = costBuf[i];
      rows[i].highlighted = false;
   }
}


void BumperInfo::fillRows()
{
   static char elastBuf[XtankBumperCount][COL_LEN];
   static char costBuf[XtankBumperCount][COL_LEN];

   for(S32 i = 0; i < XtankBumperCount; i++)
   {
      const BumperStat &bi = bumperStat[i];
      dSprintf(elastBuf[i], COL_LEN, "%.2f", bi.elasticity);
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(bi.cost)));

      rows[i].cells[0] = InputCodeManager::inputCodeToString(getKeyForIndex(i));
      rows[i].cells[1] = bi.name;
      rows[i].cells[2] = elastBuf[i];
      rows[i].cells[3] = costBuf[i];
      rows[i].highlighted = false;
   }
}


void HeatSinkInfo::fillRows()
{
   // Do nothing
}

void ArmorAllocationInfo::fillRows()
{
   for(S32 i = 0; i < VehicleSidesCount; i++)
   {

      rows[i].cells[0] = InputCodeManager::inputCodeToString(sArmorAllocationKeys[i]);
      rows[i].cells[1] = sideNames[i];
      rows[i].highlighted = false;
   }

   array<S32, VehicleSidesCount> armor{};	 // 0 initialize -- No armor at the beginning
   updateArmorAllocationMenuItems(XtankArmor::DEFAULT, armor.data());
}


// Update the points and related stats columns for rendering the armor allocation menu
void ArmorAllocationInfo::updateArmorAllocationMenuItems(XtankArmor armorType, const S32 *armor)
{
   static char ptsBuf[VehicleSidesCount][COL_LEN];

   for(S32 i = 0; i < VehicleSidesCount; i++)
   {
      dSprintf(ptsBuf[i], COL_LEN, "%d", armor[i]);
      rows[i].cells[2] = ptsBuf[i];
   }
}


// Row 0 = "None" (empty slot), rows 1..XtankWeapon::COUNT = weapon entries.
void WeaponInfo2::fillRows()
{
   static char dlyBuf[(S32)XtankWeapon::COUNT][COL_LEN];
   static char wtBuf[(S32)XtankWeapon::COUNT][COL_LEN];
   static char costBuf[(S32)XtankWeapon::COUNT][COL_LEN];

   // Row 0: "None" placeholder
   rows[0].cells[0] = InputCodeManager::inputCodeToString(KEY_0);
   rows[0].cells[1] = "None";
   rows[0].cells[2] = "--";
   rows[0].cells[3] = "--";
   rows[0].cells[4] = "--";
   rows[0].highlighted = false;

   for(S32 i = 0; i < (S32)XtankWeapon::COUNT; i++)
   {
      const XtankWeaponInfo &wi = xtankWeaponInfos[i];
      dSprintf(dlyBuf[i], COL_LEN, "%d", (S32)xtankFireDelayMs(wi));
      dSprintf(wtBuf[i], COL_LEN, "%d", wi.weight);
      dSprintf(costBuf[i], COL_LEN, "%s", cs(comma(wi.cost)));

      rows[i + 1].cells[0] = InputCodeManager::inputCodeToString(getKeyForIndex(i));
      rows[i + 1].cells[1] = wi.name;
      rows[i + 1].cells[2] = dlyBuf[i];
      rows[i + 1].cells[3] = wtBuf[i];
      rows[i + 1].cells[4] = costBuf[i];
      rows[i + 1].highlighted = false;
   }
}


// Draw overall ship preview/spec panel on the right side of the screen.
// This is intentionally aggregate-focused: highlighted-item details live in
// the active center card, while this panel shows what the ship becomes.
void VehiclePreviewRenderer::renderPreviewPanel(const XtankDesign &preview, Phase designPhase, S32 activeWaponSlot)
{
   static const S32 PNL_LEFT = 680;
   static const S32 PNL_RIGHT = 1050;
   static const S32 PNL_TOP = 20;
   static const S32 PNL_BOT = 515;
   static const S32 PNL_CX = (PNL_LEFT + PNL_RIGHT) / 2;
   static const S32 CORNER = 8;

   static const S32 TITLE_SZ = 16;
   static const S32 LINE_GAP = STAT_SZ + 5;
   static const S32 TITLE_Y = PNL_TOP + 10;
   static const S32 BODY_CY = PNL_TOP + 88;   // raised — smaller ship
   static const S32 STATS_Y = PNL_TOP + 165;  // tighter below ship
   static const S32 BUILD_Y = PNL_TOP + 390;
   static const S32 HINT_Y = PNL_BOT - 20;
   static const F32 BODY_SCALE = 1.5f;            // was 2.2f

   drawFilledFancyBox(PNL_LEFT, PNL_TOP, PNL_RIGHT, PNL_BOT, CORNER, Colors::black, 0.75f, Colors::red35);

   FontManager::pushFontContext(HelperMenuContext);
   Renderer &r = Renderer::get();

   //if(designPhase == Phase::WEAPONS)
   //{
   //   S32 slot = weaponSide;
   //   if(slot >= 0 && slot < WEAPON_SLOTS)
   //   {
   //      if(mHighlightedIndex <= 0)
   //      {
   //         preview.weapons[slot] = XtankWeapon::NONE;
   //         preview.weaponMounts[slot] = XtankMountLocation::NONE;
   //      }
   //      else if(mHighlightedIndex < mWeaponItems.size())
   //      {
   //         preview.weapons[slot] = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;
   //         XtankMountLocation preferred = (XtankMountLocation)preview.weaponMounts[slot];
   //         S32 previewBodyIdx = MAX(0, MIN((S32)preview.bodyIndex, VehicleBodyCount - 1));
   //         preview.weaponMounts[slot] = (S8)firstValidMount(previewBodyIdx, preview.weapons[slot], preferred);
   //      }
   //   }
   //}

   S32 bodyIdx = (S32)preview.body;
   S32 engineIdx = (S32)preview.engine;
   S32 treadIdx = (S32)preview.tread;
   S32 heatSinks = preview.heatSinks;

   const XtankEngineInfo &eng = xtankEngineInfos[engineIdx];
   const XtankTreadInfo &trd = xtankTreadInfos[treadIdx];
   XtankDerivedStats derived = getDerivedStats(preview);

   F32 effSpeed = derived.maxSpeed;
   F32 effRev = derived.maxReverseSpeed;
   F32 effAccel = derived.maxAccel;
   F32 effTurn = derived.maxTurnRate;
   S32 fireRatePct = preview.heatDissipation();

   S32 totalWeight = xTankBodyStats[bodyIdx].weight + eng.weight + heatSinkStat.weight * heatSinks;
   S32 totalCost = xTankBodyStats[bodyIdx].cost + eng.cost + trd.cost + heatSinkStat.cost * heatSinks;
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      XtankWeapon w = preview.weapons[i];
      if(w == XtankWeapon::NONE)
         continue;
      totalWeight += xtankWeaponInfos[(S32)w].weight;
      totalCost += xtankWeaponInfos[(S32)w].cost;
   }

   // Armor cost and weight: xtank-style scaling by body size.
   S32 armorIdx = MAX(0, MIN((S32)preview.armor, XtankArmorCount - 1));
   S32 totalArmorPoints = 0;
   for(S32 i = 0; i < 6; i++)
      totalArmorPoints += preview.armorSides[i];
   totalWeight += totalArmorPoints * xtankArmorInfos[armorIdx].weight * xTankBodyStats[bodyIdx].size;
   totalCost += totalArmorPoints * xtankArmorInfos[armorIdx].cost * xTankBodyStats[bodyIdx].size;

   // Suspension, treads, and bumpers scale by body size in xtank.
   totalCost += suspensionStat[MAX(0, MIN((S32)preview.suspension, XtankSuspensionCount - 1))].cost * xTankBodyStats[bodyIdx].size;
   totalCost += trd.cost * xTankBodyStats[bodyIdx].size;
   totalCost += bumperStat[MAX(0, MIN((S32)preview.bumper, XtankBumperCount - 1))].cost * xTankBodyStats[bodyIdx].size;

   // Specials costs and weights
   for(S32 s = 0; s < XtankSpecialCount; s++)
   {
      if(hasSpecial(preview.specials, (XtankSpecial)s))
      {
         totalWeight += xtankSpecialInfos[s].weight;
         totalCost += xtankSpecialInfos[s].cost;
      }
   }

   // Space tracking
   S32 spaceLimit = xTankBodyStats[bodyIdx].space;
   S32 totalSpace = eng.space;
   totalSpace += totalArmorPoints * xtankArmorInfos[armorIdx].space * xTankBodyStats[bodyIdx].size;
   totalSpace += heatSinkStat.space * heatSinks;
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      XtankWeapon w = preview.weapons[i];
      if(w == XtankWeapon::NONE)
         continue;
      totalSpace += xtankWeaponInfos[(S32)w].space;
   }
   for(S32 s = 0; s < XtankSpecialCount; s++)
      if(hasSpecial(preview.specials, (XtankSpecial)s))
         totalSpace += xtankSpecialInfos[s].space;

   S32 weightLimit = xTankBodyStats[bodyIdx].weightLimit;
   bool overWeight = totalWeight > weightLimit;
   bool overSpace = totalSpace > spaceLimit;

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
      const_cast<F32 *>(thrusts), 1.0f, F32(Ship::CollisionRadius), 0,
      false, false, false, false);
   renderXtankTurrets(Point(0, 0), 0, 0, 1.0f,
      xtankTurretInfos[bodyIdx], &Colors::blue, 1.0f);
   r.popMatrix();

   S32 ty = STATS_Y;


   r.setColor(Colors::cyan);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Body: %s", xtankBodyNames[bodyIdx]);                                    ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Speed: %s  Rev: %s", cs(comma(effSpeed)), cs(comma(effRev)));           ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Accel: %s  Turn: %.1f", cs(comma(effAccel)), effTurn);                  ty += LINE_GAP;
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Handling: %d  Turrets: %d", xTankBodyStats[bodyIdx].handling, xtankTurretInfos[bodyIdx].count);    ty += LINE_GAP;

   if(fireRatePct > 0)
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Fire bonus: +%d%%", fireRatePct);
   else
      drawCenteredString(PNL_CX, ty, STAT_SZ, "Fire bonus: base");
   ty += LINE_GAP;

   r.setColor(overWeight ? Colors::red : Colors::green);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Weight: %s / %s%s",
      cs(comma(totalWeight)), cs(comma(weightLimit)),
      overWeight ? " OVER!" : "");
   ty += LINE_GAP;

   r.setColor(overSpace ? Colors::red : Colors::cyan);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Space:  %s / %s%s",
      cs(comma(totalSpace)), cs(comma(spaceLimit)),
      overSpace ? " OVER!" : "");
   ty += LINE_GAP;

   r.setColor(Colors::yellow);
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Cost: %s", cs(comma(totalCost)));
   ty += LINE_GAP + 2;

   // Show weapon loadout in two columns so stats remain readable.
   r.setColor(Colors::gray45);
   drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, ty);
   ty += 6;

   ty += renderWeaponList(designPhase, activeWaponSlot, PNL_LEFT, PNL_CX, ty, preview, STAT_SZ - 2, LINE_GAP);

   ty += 6;

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

   renderFullBuildStats(PNL_CX, BUILD_Y, preview);
   //renderCarouselDots(PNL_CX, DOTS_Y);

   r.setColor(Colors::gray50);
   if(designPhase == Phase::WEAPONS)
      drawCenteredStringf(PNL_CX, HINT_Y, 11, "[Lt]/[Rt] mount  [Up]/[Dn] weapon  Tab next slot");
   else
      drawCenteredString(PNL_CX, HINT_Y, 11, "[Up]/[Dn] item  Tab/Shift-Tab phase");

   FontManager::popFontContext();
}


// Render the small weapons table on the ship preview
S32 VehiclePreviewRenderer::renderWeaponList(Phase designPhase, S32 activeWeaponSlot, S32 left, S32 cx, S32 y, const XtankDesign &preview, S32 fontSz, S32 lineGap)
{
   Renderer &r = Renderer::get();

   r.setColor(Colors::gray70);
   drawCenteredString(cx, y, fontSz - 1, "Weapons");
   y += lineGap;

   const S32 weaponRows = (WEAPON_SLOTS + 1) / 2;

   // Build all label strings first and measure their widths per column,
   // so we can compute the true block width and center it on the panel.
   char buffers[WEAPON_SLOTS][128];
   S32  colWidth[2] = { 0, 0 };
   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      XtankWeapon w = preview.weapons[i];
      XtankMountLocation mount = preview.weaponMounts[i];
      const char *mountName = getMountLabel(mount);
      if(w != XtankWeapon::NONE)
         dSprintf(buffers[i], sizeof(buffers[i]), "#%d %s: %s", i + 1, mountName, xtankWeaponInfos[(S32)w].name);
      else
         dSprintf(buffers[i], sizeof(buffers[i]), "#%d Empty Slot", i + 1, mountName);

      S32 col = (i < weaponRows) ? 0 : 1;
      colWidth[col] = MAX(colWidth[col], getStringWidth(fontSz, buffers[i]));
   }

   const S32 midMargin = 14;
   const S32 blockWidth = colWidth[0] + midMargin + colWidth[1];
   const S32 col0X = cx - blockWidth / 2;
   const S32 col1X = col0X + colWidth[0] + midMargin;

   for(S32 i = 0; i < WEAPON_SLOTS; i++)
   {
      bool slotIsActive = (designPhase == Phase::WEAPONS && activeWeaponSlot == i);
      r.setColor(slotIsActive ? Colors::overlayMenuSelectedItemColor : Colors::cyan);

      S32 col = (i < weaponRows) ? 0 : 1;
      S32 row = i % weaponRows;
      drawString((col == 0) ? col0X : col1X, y + row * lineGap, fontSz, buffers[i]);
   }
   return (weaponRows + 1) * lineGap;
}



// Draw a "Current  Build Stats" section showing effective vehicle performance
// with the given (potentially preview) component indices applied.
void VehiclePreviewRenderer::renderFullBuildStats(S32 cx, S32 y, const XtankDesign &preview)
{
   XtankDerivedStats derived = getDerivedStats(preview);
   S32 hs = CLAMP(preview.heatSinks, 0, MAX_HEAT_SINKS);
   S32 fireRatePct = preview.heatDissipation();

   Renderer &r = Renderer::get();

   // Section divider
   const S32 DIVIDER_MARGIN = 80;
   r.setColor(Colors::gray40);
   S32 divY = y - 4;
   drawHorizLine(cx - DIVIDER_MARGIN, cx + DIVIDER_MARGIN, divY);

   // Section label
   r.setColor(Colors::gray50);
   drawCenteredString(cx, divY - STAT_SZ - 2, STAT_SZ, "CURRENT BUILD");

   // Stat lines
   S32 ty = y + 4;
   r.setColor(Colors::green);
   drawCenteredStringf(cx, ty, STAT_SZ, "Spd: %d  Rev: %d", (S32)derived.maxSpeed, (S32)derived.maxReverseSpeed);
   ty += GAP;
   drawCenteredStringf(cx, ty, STAT_SZ, "Acc: %d  Turn: %.1f", (S32)derived.maxAccel, derived.maxTurnRate);
   ty += GAP;
   if(fireRatePct > 0)
      drawCenteredStringf(cx, ty, STAT_SZ, "Fire rate: +%d%%", fireRatePct);
   else
      drawCenteredString(cx, ty, STAT_SZ, "Fire rate: base");
}

} /* namespace Zap */
#undef UNSEL_COLOR
