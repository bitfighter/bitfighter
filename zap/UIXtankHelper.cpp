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

static const S32 kXtankMaxWeaponSlots = XtankMaxWeapons;

struct XtankDerivedStats
{
   F32 maxSpeed;
   F32 maxReverseSpeed;
   F32 maxAccel;
   F32 maxTurnRate;
};

// Returns max total armor units a bare hull can carry for the specified armor type,
// limited by both weight limit and space limit (xtank-style size scaling).
static S32 getBodyArmorCapacityUnits(S32 bodyIdx, S32 armorIdx)
{
   bodyIdx = MAX(0, MIN(bodyIdx, XtankBodyCount - 1));
   armorIdx = MAX(0, MIN(armorIdx, XtankArmorCount - 1));

   const XtankBodyInfo &body = body_stat[bodyIdx];
   const XtankArmorInfo &armor = xtankArmorInfos[armorIdx];

   S32 weightBudget = MAX(0, body.weightLimit - body.weight);
   S32 unitWeight = armor.weight * body.size;
   S32 unitSpace  = armor.space * body.size;

   S32 byWeight = (unitWeight > 0) ? (weightBudget / unitWeight) : S32_MAX;
   S32 bySpace  = (unitSpace  > 0) ? (body.space   / unitSpace)  : S32_MAX;

   return MAX(0, MIN(byWeight, bySpace));
}


// Returns how many additional armor units the design can still accept, accounting
// for weight and space already consumed by all non-armor components.
static S32 getRemainingArmorUnits(const XtankDesign &design)
{
   S32 bodyIdx  = MAX(0, MIN((S32)design.bodyIndex,  XtankBodyCount  - 1));
   S32 armorIdx = MAX(0, MIN((S32)design.armorType,  XtankArmorCount - 1));
   S32 engIdx   = MAX(0, MIN((S32)design.engineType, XtankEngineCount - 1));
   S32 heatSinks = MAX(XtankHeatSinkMin, MIN((S32)design.heatSinkCount, XtankHeatSinkMax));

   const XtankBodyInfo   &body  = body_stat[bodyIdx];
   const XtankArmorInfo  &armor = xtankArmorInfos[armorIdx];
   const XtankEngineInfo &eng   = xtankEngineInfos[engIdx];

   // Weight used by non-armor components.
   S32 otherWeight = body.weight + eng.weight + heatSinkStat.weight * heatSinks;
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = design.weapons[i];
      if(w != XtankWeaponNone && (S32)w >= 0 && (S32)w < XtankWeaponCount)
         otherWeight += xtankWeaponInfos[(S32)w].weight;
   }
   for(S32 s = 0; s < XtankSpecialCount; s++)
      if(hasSpecial(design.specials, (XtankSpecial)s))
         otherWeight += xtankSpecialInfos[s].weight;

   // Space used by non-armor components.
   S32 otherSpace = eng.space + heatSinkStat.space * heatSinks;
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = design.weapons[i];
      if(w != XtankWeaponNone && (S32)w >= 0 && (S32)w < XtankWeaponCount)
         otherSpace += xtankWeaponInfos[(S32)w].space;
   }
   for(S32 s = 0; s < XtankSpecialCount; s++)
      if(hasSpecial(design.specials, (XtankSpecial)s))
         otherSpace += xtankSpecialInfos[s].space;

   S32 unitWeight = armor.weight * body.size;
   S32 unitSpace  = armor.space  * body.size;

   S32 weightBudget = MAX(0, body.weightLimit - otherWeight);
   S32 spaceBudget  = MAX(0, body.space       - otherSpace);

   S32 byWeight = (unitWeight > 0) ? (weightBudget / unitWeight) : S32_MAX;
   S32 bySpace  = (unitSpace  > 0) ? (spaceBudget  / unitSpace)  : S32_MAX;

   return MAX(0, MIN(byWeight, bySpace));
}


// Derive movement values from original xtank stats (body_stat + engine/treads).
static XtankDerivedStats getDerivedStats(const XtankDesign &design)
{
   static const F32 XTANK_FPS = 20.0f;
   static const F32 BF_SCALE  = 1.5f;
   static const F32 TURN_SCALE = 3.5f;
   static const F32 MIN_EFFECTIVE_HANDLING = 1.0f;

   S32 bodyIdx = MAX(0, MIN((S32)design.bodyIndex, XtankBodyCount - 1));
   S32 engineIdx = MAX(0, MIN((S32)design.engineType, XtankEngineCount - 1));
   S32 treadIdx = MAX(0, MIN((S32)design.treadType, XtankTreadCount - 1));
   S32 armorIdx = MAX(0, MIN((S32)design.armorType, XtankArmorCount - 1));
   S32 suspensionIdx = MAX(0, MIN((S32)design.suspensionType, XtankSuspensionCount - 1));
   S32 heatSinks = MAX(XtankHeatSinkMin, MIN((S32)design.heatSinkCount, XtankHeatSinkMax));

   const XtankBodyInfo   &body = body_stat[bodyIdx];
   const XtankEngineInfo &engine = xtankEngineInfos[engineIdx];
   const XtankTreadInfo  &tread = xtankTreadInfos[treadIdx];
   const XtankArmorInfo  &armor = xtankArmorInfos[armorIdx];
   const SuspensionStat  &suspension = suspensionStat[suspensionIdx];

   S32 weaponWeight = 0;
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = design.weapons[i];
      if(w != XtankWeaponNone && (S32)w >= 0 && (S32)w < XtankWeaponCount)
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
   bool isHover = ((S32)design.treadType == (S32)TREAD_HOVER);

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


static const char *getMountLabel(XtankMountLocation mount)
{
   switch(mount)
   {
      case MOUNT_TURRET1: return "Turret 1";
      case MOUNT_TURRET2: return "Turret 2";
      case MOUNT_TURRET3: return "Turret 3";
      case MOUNT_TURRET4: return "Turret 4";
      case MOUNT_FRONT:   return "Front";
      case MOUNT_BACK:    return "Back";
      case MOUNT_LEFT:    return "Left";
      case MOUNT_RIGHT:   return "Right";
      default:            return "None";
   }
}


static S32 getXtankMountBit(XtankMountLocation mount)
{
   switch(mount)
   {
      case MOUNT_TURRET1:
      case MOUNT_TURRET2:
      case MOUNT_TURRET3:
      case MOUNT_TURRET4: return M_TURRET;
      case MOUNT_FRONT:   return M_FRONT;
      case MOUNT_BACK:    return M_BACK;
      case MOUNT_LEFT:    return M_LEFT;
      case MOUNT_RIGHT:   return M_RIGHT;
      default:            return 0;
   }
}


static bool bodySupportsMount(S32 bodyIdx, XtankMountLocation mount)
{
   if(bodyIdx < 0 || bodyIdx >= XtankBodyCount)
      return false;

   if(mount >= MOUNT_TURRET1 && mount <= MOUNT_TURRET4)
      return ((S32)mount - (S32)MOUNT_TURRET1) < xtankTurretInfos[bodyIdx].count;

   return mount == MOUNT_FRONT || mount == MOUNT_BACK || mount == MOUNT_LEFT || mount == MOUNT_RIGHT;
}


static bool mountCompatible(S32 bodyIdx, XtankWeapon weapon, XtankMountLocation mount)
{
   if((S32)weapon < 0 || (S32)weapon >= XtankWeaponCount)
      return false;
   if(mount < 0 || mount >= XtankMountCount)
      return false;
   if(!bodySupportsMount(bodyIdx, mount))
      return false;
   return (xtankWeaponInfos[(S32)weapon].mount & getXtankMountBit(mount)) != 0;
}


static XtankMountLocation firstValidMount(S32 bodyIdx, XtankWeapon weapon, XtankMountLocation preferred)
{
   if(mountCompatible(bodyIdx, weapon, preferred))
      return preferred;

   for(S32 m = 0; m < XtankMountCount; m++)
   {
      XtankMountLocation mount = (XtankMountLocation)m;
      if(mountCompatible(bodyIdx, weapon, mount))
         return mount;
   }

   return XtankMountNone;
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
   mWeaponMountButtonsWidth      = 0;
   mWeaponMountItemsDisplayWidth = 0;
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
      XtankDesign bodyDefaults;
      bodyDefaults.initForBody(i);
      XtankDerivedStats stats = getDerivedStats(bodyDefaults);
      S32 turrets = xtankTurretInfos[i].count;
      S32 armorIdx = MAX(0, MIN((S32)mDesignInProgress.armorType, XtankArmorCount - 1));
      S32 armorCap = getBodyArmorCapacityUnits(i, armorIdx);

      // Speed class string (rough bands)
      const char *spdClass = (stats.maxSpeed >= 550.0f) ? "Fast" :
                             (stats.maxSpeed >= 480.0f) ? "Medium"  : "Slow";

      // We store per-item help text in persistent strings (one per body).
      // Use static storage so the c_str() pointers remain valid.
      static string helpTexts[XtankBodyCount];
      helpTexts[i] = string("SPD:") + spdClass +
                     "  HNDL:" + itos(body_stat[i].handling) +
                     "  ACAP:" + itos(armorCap) +
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


// Build the 6-item armor-sides phase list (Front / Back / Left / Right / Top / Bottom).
// Items have no hotkeys; point values are edited live with +/- in processInputCode.
void UIXtankHelper::buildArmorSidesItems()
{
   static const char *sideNames[6] = { "Front", "Back", "Left", "Right", "Top", "Bottom" };
   mArmorSidesItems.clear();
   for(S32 i = 0; i < 6; i++)
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


// Returns how many weapon slots (excluding `excludeSlot`) are assigned to `mount`.
S32 UIXtankHelper::countWeaponsOnMount(XtankMountLocation mount, S32 excludeSlot) const
{
   S32 count = 0;
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      if(i == excludeSlot)
         continue;
      if(mDesignInProgress.weapons[i] != XtankWeaponNone &&
         (XtankMountLocation)mDesignInProgress.weaponMounts[i] == mount)
         count++;
   }
   return count;
}

void UIXtankHelper::buildWeaponMountItems()
{
   mWeaponMountItems.clear();

   // Slot can be empty, in which case mount is None.
   {
      OverlayMenuItem item;
      item.key                 = KEY_0;
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)XtankMountCount;
      item.name                = "None";
      item.itemColor           = UNSEL_COLOR;
      item.help                = "No mount (empty weapon slot)";
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponMountItems.push_back(item);
   }

   for(S32 m = 0; m < XtankMountCount; m++)
   {
      // A mount is available if fewer than 2 weapons (from other slots) already use it.
      bool full = countWeaponsOnMount((XtankMountLocation)m, mWeaponSide) >= 2;

      OverlayMenuItem item;
      item.key                 = getKeyForIndex(m);
      item.button              = KEY_NONE;
      item.showOnMenu          = !full;
      item.itemIndex           = (U32)m;
      item.name                = getMountLabel((XtankMountLocation)m);
      item.itemColor           = UNSEL_COLOR;
      item.help                = full ? "Mount full (2 weapons max)" : "";
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponMountItems.push_back(item);
   }
}


void UIXtankHelper::normalizeWeaponPanels()
{
   // Compact all active weapons to the front so the weapon panel sequence is
   // always: [selected weapons...] + [one trailing None], capped at 6.
   XtankWeapon compactWeapons[kXtankMaxWeaponSlots];
   S8 compactMounts[kXtankMaxWeaponSlots];
   S32 writeIdx = 0;

   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = mDesignInProgress.weapons[i];
      if(w == XtankWeaponNone)
         continue;
      compactWeapons[writeIdx] = w;
      compactMounts[writeIdx] = mDesignInProgress.weaponMounts[i];
      writeIdx++;
   }

   for(S32 i = 0; i < writeIdx; i++)
   {
      mDesignInProgress.weapons[i] = compactWeapons[i];
      mDesignInProgress.weaponMounts[i] = compactMounts[i];
   }
   for(S32 i = writeIdx; i < kXtankMaxWeaponSlots; i++)
   {
      mDesignInProgress.weapons[i] = XtankWeaponNone;
      mDesignInProgress.weaponMounts[i] = (S8)XtankMountNone;
   }

   // Show one menu panel per selected weapon, plus one "No weapon" panel,
   // up to the max slot count.
   if(writeIdx >= kXtankMaxWeaponSlots)
      mSlotCount = kXtankMaxWeaponSlots;
   else
      mSlotCount = writeIdx + 1;

   if(mSlotCount < 1)
      mSlotCount = 1;
   if(mWeaponSide >= mSlotCount)
      mWeaponSide = mSlotCount - 1;
   if(mWeaponSide < 0)
      mWeaponSide = 0;
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
   buildHeatSinkItems();
   buildWeaponItems();
   buildWeaponMountItems();

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
   mWeaponMountButtonsWidth      = getButtonWidth(mWeaponMountItems.address(), mWeaponMountItems.size());
   mWeaponMountItemsDisplayWidth = getMaxItemWidth(mWeaponMountItems.address(), mWeaponMountItems.size());

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
   else if(mPhase == PHASE_WEAPONS)   updateItemColors(mWeaponItems);
   else                               updateItemColors(mWeaponMountItems);

   // Compute transition fraction (0.0 to 1.0)
   F32 transitionFraction = 0.0f;
   if(mTransitionTimer.getCurrent() > 0)
      transitionFraction = MIN(1.0f, mTransitionTimer.getFraction());

   renderFloatingMenus(transitionFraction);   renderPreviewPanel();
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
         mDesignInProgress.bodyIndex = (S8)bodyIdx;

         // Drop any weapons that exceed the new body's turret count, then
         // compact surviving weapons to the front so slot indices stay valid.
         S32 turretCount = xtankTurretInfos[bodyIdx].count;
         for(S32 j = turretCount; j < kXtankMaxWeaponSlots; j++)
         {
            mDesignInProgress.weapons[j]      = XtankWeaponNone;
            mDesignInProgress.weaponMounts[j] = (S8)XtankMountNone;
         }
         normalizeWeaponPanels();
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
      // Armor side values are edited live with +/- and arrow shortcuts.
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
   else if(mPhase == PHASE_WEAPONS)
   {
      if(mWeaponSide >= 0 && mWeaponSide < kXtankMaxWeaponSlots &&
         mHighlightedIndex >= 0 && mHighlightedIndex < mWeaponItems.size())
      {
         XtankWeapon selectedWeapon = XtankWeaponNone;
         if(mHighlightedIndex > 0)
            selectedWeapon = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;

         XtankWeapon currentWeapon = mDesignInProgress.weapons[mWeaponSide];
         S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));

         // Compute how many panels are currently filled with weapons.
         S32 activeCount = 0;
         for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
            if(mDesignInProgress.weapons[i] != XtankWeaponNone)
               activeCount++;

         if(selectedWeapon == XtankWeaponNone)
         {
            // Remove current weapon panel (shift down) only if this panel is armed.
            if(currentWeapon != XtankWeaponNone)
            {
               for(S32 i = mWeaponSide; i < kXtankMaxWeaponSlots - 1; i++)
               {
                  mDesignInProgress.weapons[i] = mDesignInProgress.weapons[i + 1];
                  mDesignInProgress.weaponMounts[i] = mDesignInProgress.weaponMounts[i + 1];
               }
               mDesignInProgress.weapons[kXtankMaxWeaponSlots - 1] = XtankWeaponNone;
               mDesignInProgress.weaponMounts[kXtankMaxWeaponSlots - 1] = (S8)XtankMountNone;
            }
         }
         else
         {
            if(currentWeapon == XtankWeaponNone && activeCount < kXtankMaxWeaponSlots)
            {
               // Insert a new weapon panel at this position (shift up).
               for(S32 i = kXtankMaxWeaponSlots - 1; i > mWeaponSide; i--)
               {
                  mDesignInProgress.weapons[i] = mDesignInProgress.weapons[i - 1];
                  mDesignInProgress.weaponMounts[i] = mDesignInProgress.weaponMounts[i - 1];
               }
            }

            mDesignInProgress.weapons[mWeaponSide] = selectedWeapon;
            XtankMountLocation preferred = (XtankMountLocation)mDesignInProgress.weaponMounts[mWeaponSide];
            mDesignInProgress.weaponMounts[mWeaponSide] = (S8)firstValidMount(bodyIdx, selectedWeapon, preferred);
         }

         normalizeWeaponPanels();
      }
   }
   else if(mPhase == PHASE_WEAPON_MOUNT)
   {
      if(mWeaponSide >= 0 && mWeaponSide < kXtankMaxWeaponSlots &&
         mHighlightedIndex >= 0 && mHighlightedIndex < mWeaponMountItems.size())
      {
         XtankWeapon w = mDesignInProgress.weapons[mWeaponSide];
         S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));

         if(mHighlightedIndex == 0 || w == XtankWeaponNone)
         {
            mDesignInProgress.weaponMounts[mWeaponSide] = (S8)XtankMountNone;
         }
         else
         {
            XtankMountLocation mount = (XtankMountLocation)(S32)mWeaponMountItems[mHighlightedIndex].itemIndex;
            // Reject the mount if it already has 2 weapons from other slots.
            if(countWeaponsOnMount(mount, mWeaponSide) >= 2)
               mount = XtankMountNone;
            if(mount != XtankMountNone && mountCompatible(bodyIdx, w, mount))
               mDesignInProgress.weaponMounts[mWeaponSide] = (S8)mount;
            else
               mDesignInProgress.weaponMounts[mWeaponSide] = (S8)firstValidMount(bodyIdx, w, mount);
         }
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

   // TAB / SHIFT-TAB: the only menu navigation.
   // In weapon phase, they move between weapon-number panels.
   if(inputCode == KEY_TAB)
   {
      commitHighlightedSelection();

      if(mPhase == PHASE_WEAPONS)
      {
         if(shiftDown)
         {
            if(mWeaponSide > 0)
            {
               mTransitionFromPhase = mPhase;
               mTransitionForward = false;
               mTransitionTimer.reset(250);
               mWeaponSide--;
               setHighlightedIndexForPhase(PHASE_WEAPONS);
               return true;
            }
            navigateBackward();
            return true;
         }

         if(mWeaponSide < MAX(0, mSlotCount - 1))
         {
            mTransitionFromPhase = mPhase;
            mTransitionForward = true;
            mTransitionTimer.reset(250);
            mWeaponSide++;
            setHighlightedIndexForPhase(PHASE_WEAPONS);
            return true;
         }
         navigateForward();
         return true;
      }

      if(shiftDown)
         navigateBackward();
      else
         navigateForward();
      return true;
   }

   // LEFT/BACKSPACE and RIGHT are not used for phase navigation.
   // In the weapon phase they toggle mount choice for the current weapon panel.
   // In armor-allocation phase they mirror '-' and '+' respectively.
   if(inputCode == KEY_LEFT || inputCode == KEY_BACKSPACE)
   {
      if(mPhase == PHASE_WEAPONS)
      {
         // Apply current highlighted weapon selection before toggling mount.
         commitHighlightedSelection();
         XtankWeapon w = mDesignInProgress.weapons[mWeaponSide];
         S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
         if((S32)w >= 0 && (S32)w < XtankWeaponCount)
         {
            S32 start = (S32)mDesignInProgress.weaponMounts[mWeaponSide];
            if(start < 0 || start >= XtankMountCount)
               start = 0;
            for(S32 step = 1; step <= XtankMountCount; step++)
            {
               S32 idx = (start - step + XtankMountCount * 4) % XtankMountCount;
               XtankMountLocation mount = (XtankMountLocation)idx;
               if(mountCompatible(bodyIdx, w, mount))
               {
                  mDesignInProgress.weaponMounts[mWeaponSide] = (S8)mount;
                  break;
               }
            }
         }
         return true;
      }
      if(mPhase == PHASE_ARMOR_SIDES)
      {
         S32 side = mHighlightedIndex;
         if(side >= 0 && side < 6 && mDesignInProgress.armorSides[side] > 0)
            mDesignInProgress.armorSides[side]--;
         return true;
      }
   }

   if(inputCode == KEY_RIGHT)
   {
      if(mPhase == PHASE_WEAPONS)
      {
         // Apply current highlighted weapon selection before toggling mount.
         commitHighlightedSelection();
         XtankWeapon w = mDesignInProgress.weapons[mWeaponSide];
         S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
         if((S32)w >= 0 && (S32)w < XtankWeaponCount)
         {
            S32 start = (S32)mDesignInProgress.weaponMounts[mWeaponSide];
            if(start < 0 || start >= XtankMountCount)
               start = 0;
            for(S32 step = 1; step <= XtankMountCount; step++)
            {
               S32 idx = (start + step + XtankMountCount * 4) % XtankMountCount;
               XtankMountLocation mount = (XtankMountLocation)idx;
               if(mountCompatible(bodyIdx, w, mount))
               {
                  mDesignInProgress.weaponMounts[mWeaponSide] = (S8)mount;
                  break;
               }
            }
         }
         return true;
      }
      if(mPhase == PHASE_ARMOR_SIDES)
      {
         S32 side = mHighlightedIndex;
         if(side >= 0 && side < 6 && mDesignInProgress.armorSides[side] < 255)
            mDesignInProgress.armorSides[side]++;
         return true;
      }
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
            mHighlightedIndex = i;
            navigateForward();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_ARMOR_SIDES)
   {
      // '+' / '=' : add 1 point to the highlighted side (xtank-style independent sides).
      if(inputCode == KEY_EQUALS)
      {
         S32 side = mHighlightedIndex;
         if(side >= 0 && side < 6 && mDesignInProgress.armorSides[side] < 255)
            mDesignInProgress.armorSides[side]++;
         return true;
      }
      // '-' : remove 1 point from the highlighted side.
      if(inputCode == KEY_MINUS)
      {
         S32 side = mHighlightedIndex;
         if(side >= 0 && side < 6 && mDesignInProgress.armorSides[side] > 0)
            mDesignInProgress.armorSides[side]--;
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
      // Hotkeys apply through commitHighlightedSelection so panel insertion/removal
      // semantics are identical to tabbing away from the panel.
      for(S32 i = 0; i < mWeaponItems.size(); i++)
      {
         if(inputCode == mWeaponItems[i].key)
         {
            mHighlightedIndex = i;
            commitHighlightedSelection();
            setHighlightedIndexForPhase(PHASE_WEAPONS);
            return true;
         }
      }
   }
   else if(mPhase == PHASE_WEAPON_MOUNT)
   {
      for(S32 i = 0; i < mWeaponMountItems.size(); i++)
      {
         if(inputCode == mWeaponMountItems[i].key)
         {
            mHighlightedIndex = i;
            commitHighlightedSelection();
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
   if(TOTAL_PHASES < 1)
      return;

   // Start transition animation
   mTransitionFromPhase = mPhase;
   mTransitionForward = true;
   mTransitionTimer.reset(250);  // 250ms transition

   mPhase = (mPhase + 1) % TOTAL_PHASES;

   // When entering weapon phases from non-weapon phases, reset to slot 0.
   if((mPhase == PHASE_WEAPONS || mPhase == PHASE_WEAPON_MOUNT) &&
      !(mTransitionFromPhase == PHASE_WEAPONS || mTransitionFromPhase == PHASE_WEAPON_MOUNT))
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

   // When entering weapon phases from non-weapon phases, start at last slot.
   if((mPhase == PHASE_WEAPONS || mPhase == PHASE_WEAPON_MOUNT) &&
      !(mTransitionFromPhase == PHASE_WEAPONS || mTransitionFromPhase == PHASE_WEAPON_MOUNT))
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
   if(phase == PHASE_WEAPONS)     return getTotalDisplayWidth(mWeaponButtonsWidth,      mWeaponItemsDisplayWidth);
   return getTotalDisplayWidth(mWeaponMountButtonsWidth, mWeaponMountItemsDisplayWidth);
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
   else if(phase == PHASE_WEAPONS)
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
   else if(phase == PHASE_WEAPON_MOUNT)
   {
      S32 slot = mWeaponSide;
      if(slot < 0 || slot >= kXtankMaxWeaponSlots)
      {
         mHighlightedIndex = 0;
         return;
      }
      // Rebuild mount items so full mounts are greyed out for this specific slot.
      buildWeaponMountItems();

      XtankWeapon w = mDesignInProgress.weapons[slot];
      if(w == XtankWeaponNone)
      {
         mHighlightedIndex = 0;
         return;
      }

      XtankMountLocation mount = (XtankMountLocation)mDesignInProgress.weaponMounts[slot];
      mHighlightedIndex = 0;
      for(S32 i = 1; i < mWeaponMountItems.size(); i++)
      {
         if((S32)mWeaponMountItems[i].itemIndex == (S32)mount)
         {
            mHighlightedIndex = i;
            return;
         }
      }

      S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
      XtankMountLocation fallback = firstValidMount(bodyIdx, w, MOUNT_TURRET1);
      for(S32 i = 1; i < mWeaponMountItems.size(); i++)
      {
         if((S32)mWeaponMountItems[i].itemIndex == (S32)fallback)
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
   if(phase == PHASE_WEAPONS)     return &mWeaponItems;
   return &mWeaponMountItems;
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
      dSprintf(buf, sizeof(buf), "F:%d B:%d L:%d R:%d T:%d Bo:%d",
               (S32)mDesignInProgress.armorSides[0],
               (S32)mDesignInProgress.armorSides[1],
               (S32)mDesignInProgress.armorSides[2],
               (S32)mDesignInProgress.armorSides[3],
               (S32)mDesignInProgress.armorSides[4],
               (S32)mDesignInProgress.armorSides[5]);
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
   if(phase == PHASE_WEAPON_MOUNT)
   {
      S32 slot = MAX(0, MIN(mWeaponSide, kXtankMaxWeaponSlots - 1));
      return std::string("#") + itos(slot + 1) + " " + getMountLabel((XtankMountLocation)mDesignInProgress.weaponMounts[slot]);
   }
   return "";
}


static const char* getArmorClassName(S32 defenseValue)
{
   switch (defenseValue)
   {
   case 0: return "Light";
   case 1: return "Standard";
   case 2: return "Heavy";
   case 3: return "Super";
   default: return "?";
   }
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
      XtankDesign bodyDefaults;
      bodyDefaults.initForBody(idx);
      XtankDerivedStats stats = getDerivedStats(bodyDefaults);
      S32 armorIdx = MAX(0, MIN((S32)mDesignInProgress.armorType, XtankArmorCount - 1));
      S32 armorCap = getBodyArmorCapacityUnits(idx, armorIdx);
      drawStringf(left, y, STAT_SZ, "Speed: %s", cs(comma(stats.maxSpeed)));              y += GAP;
      drawStringf(left, y, STAT_SZ, "Reverse: %s", cs(comma(stats.maxReverseSpeed)));      y += GAP;
      drawStringf(left, y, STAT_SZ, "Accel: %s", cs(comma(stats.maxAccel)));               y += GAP;
      drawStringf(left, y, STAT_SZ, "Turn: %.1f", stats.maxTurnRate);                      y += GAP;
      drawStringf(left, y, STAT_SZ, "Drag: %.2f", body_stat[idx].drag);                    y += GAP;
      drawStringf(left, y, STAT_SZ, "Handling: %d", body_stat[idx].handling);               y += GAP;
      drawStringf(left, y, STAT_SZ, "Turrets: %d", xtankTurretInfos[idx].count);     y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(body_stat[idx].weight))); y += GAP;

      r.setColor(Color(0.7f * alpha, 0.7f * alpha, 0.7f * alpha));
      drawString(left, y, STAT_SZ, "Limits");                                          y += GAP;

      r.setColor(Color(0.0f, alpha, alpha));
      drawStringf(left + 10, y, STAT_SZ, "Weight: %s", cs(comma(body_stat[idx].weightLimit))); y += GAP;
      drawStringf(left + 10, y, STAT_SZ, "Space: %s", cs(comma(body_stat[idx].space)));         y += GAP;
      drawStringf(left + 10, y, STAT_SZ, "Armor cap: %d", armorCap);                             y += GAP;

      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(body_stat[idx].cost)));       y += GAP;
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
      // Table: Name | Class | Wt | Space | Cost
      S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
      float bodySize = body_stat[bodyIdx].size;

      // Column positions: measure widest labels for alignment
      S32 col1 = left;  // Name
      S32 col2 = col1 + getStringWidth(STAT_SZ, "Hardened Steel") + 4;  // Class
      S32 col3 = col2 + getStringWidth(STAT_SZ, "Standard") + 4;  // Wt
      S32 col4 = col3 + 25;  // Space
      S32 col5 = col4 + 25;  // Cost

      // Column headers
      r.setColor(Color(0.5f * alpha, 0.5f * alpha, 0.5f * alpha));
      drawString(col1, y, STAT_SZ, "Name");
      drawString(col2, y, STAT_SZ, "Class");
      drawString(col3, y, STAT_SZ, "Wt");
      drawString(col4, y, STAT_SZ, "Sp");
      drawString(col5, y, STAT_SZ, "Cost");
      y += GAP;

      // One row per armor type
      for(S32 i = 0; i < XtankArmorCount; i++)
      {
         bool hl = (i == mHighlightedIndex);
         r.setColor(hl ? Colors::yellow : Colors::overlayMenuUnselectedItemColor);

         const XtankArmorInfo &ai = xtankArmorInfos[i];

         drawString(col1, y, STAT_SZ, ai.name);

         r.setColor(hl ? Colors::yellow : Color(0.0f, alpha, alpha));
         drawString(col2, y, STAT_SZ, getArmorClassName(ai.defense));

         char buf[32];
         dSprintf(buf, sizeof(buf), "%d", ai.weight);
         S32 w = getStringWidth(STAT_SZ, buf);
         drawString(col3 + 18 - w, y, STAT_SZ, buf);

         dSprintf(buf, sizeof(buf), "%d", ai.space);
         w = getStringWidth(STAT_SZ, buf);
         drawString(col4 + 18 - w, y, STAT_SZ, buf);

         dSprintf(buf, sizeof(buf), "%s", cs(comma(ai.cost)));
         drawString(col5, y, STAT_SZ, buf);

         y += GAP;
      }
   }
   else if(mPhase == PHASE_ARMOR_SIDES)
   {
      S32 armorIdx = MAX(0, MIN((S32)mDesignInProgress.armorType, XtankArmorCount - 1));
      S32 bodyIdx  = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
      const XtankArmorInfo &ai = xtankArmorInfos[armorIdx];
      float bodySize = body_stat[bodyIdx].size;

      S32 total = 0;
      for(S32 i = 0; i < 6; i++)
         total += (S32)mDesignInProgress.armorSides[i];

      S32 totalWeight = (S32)(total * ai.weight * bodySize);
      S32 totalCost   = (S32)(total * ai.cost   * bodySize);
      S32 remaining   = getRemainingArmorUnits(mDesignInProgress);

      // Table: hint header
      r.setColor(Color(0.7f * alpha, 0.7f * alpha, 0.7f * alpha));
      drawString(left, y, STAT_SZ, "+/- to adjust");  y += GAP;

      // Measure the widest side label so pts/def columns line up.
      static const char *sideLabels[6] = { "Front", "Back", "Left", "Right", "Top", "Bottom" };
      S32 labelColW = getStringWidth(STAT_SZ, "Bottom") + 6;
      static const S32 NUM_COL_W = 28;
      S32 colPts = left + labelColW;
      S32 colDef = colPts + NUM_COL_W;

      // Column headings
      r.setColor(Color(0.5f * alpha, 0.5f * alpha, 0.5f * alpha));
      drawString(left,   y, STAT_SZ, "Side");
      drawString(colPts, y, STAT_SZ, "Pts");
      drawString(colDef, y, STAT_SZ, "Def");
      y += GAP;

      // One row per side
      for(S32 i = 0; i < 6; i++)
      {
         S32 pts = (S32)mDesignInProgress.armorSides[i];
         bool hl = (i == mHighlightedIndex);
         r.setColor(hl ? Colors::yellow : Color(0.0f, alpha, alpha));

         drawString(left, y, STAT_SZ, sideLabels[i]);

         char numBuf[16];
         dSprintf(numBuf, sizeof(numBuf), "%d", pts);
         S32 ptsW = getStringWidth(STAT_SZ, numBuf);
         drawString(colPts + NUM_COL_W - ptsW, y, STAT_SZ, numBuf);

         if(pts > 0)
         {
            dSprintf(numBuf, sizeof(numBuf), "%d", pts * ai.defense);
            S32 defW = getStringWidth(STAT_SZ, numBuf);
            drawString(colDef + NUM_COL_W - defW, y, STAT_SZ, numBuf);
         }
         y += GAP;
      }

      // Totals at the bottom
      y += 2;
      r.setColor(Color(0.4f * alpha, 0.4f * alpha, 0.4f * alpha));
      drawString(left, y, STAT_SZ, "---");  y += GAP;
      r.setColor(Color(0.0f, alpha, alpha));
      drawStringf(left, y, STAT_SZ, "Total:  %d pts", total);              y += GAP;
      drawStringf(left, y, STAT_SZ, "Avail:  %d units", remaining);        y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(totalWeight))); y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost:   %s", cs(comma(totalCost)));   y += GAP;
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
      drawStringf(left, y, STAT_SZ, "Fire rate +%:d%%", S32((1.0f - xtankHeatSinkFireDelayMult(n)) * 100.0f + 0.5f)); y += GAP;
      drawStringf(left, y, STAT_SZ, "Weight: %s", cs(comma(heatSinkStat.weight * n)));                                y += GAP;
      drawStringf(left, y, STAT_SZ, "Cost: %s", cs(comma(heatSinkStat.cost * n)));                                    y += GAP;
   }
   else if(mPhase == PHASE_WEAPON_MOUNT)
   {
      S32 slot = MAX(0, MIN(mWeaponSide, kXtankMaxWeaponSlots - 1));
      XtankWeapon w = mDesignInProgress.weapons[slot];
      if(w == XtankWeaponNone || (S32)w < 0 || (S32)w >= XtankWeaponCount)
         drawString(left, y, STAT_SZ, "No weapon in slot");
      else
      {
         S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
         XtankMountLocation mount = (XtankMountLocation)mDesignInProgress.weaponMounts[slot];
         drawStringf(left, y, STAT_SZ, "Weapon: %s", xtankWeaponInfos[(S32)w].name); y += GAP;
         drawStringf(left, y, STAT_SZ, "Mount: %s", getMountLabel(mount));            y += GAP;
         drawStringf(left, y, STAT_SZ, "Valid: %s", mountCompatible(bodyIdx, w, mount) ? "Yes" : "No"); y += GAP;
      }
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
void UIXtankHelper::renderCard(S32 left, S32 top, S32 right, S32 bot, S32 phase, F32 cf, S32 weaponSideOverride) const
{
   static const S32 CORNER    = 10;
   static const S32 ITEM_SZ   = 13;   // fixed item font size, same on ALL cards
   static const S32 TITLE_SZ  = 16;   // fixed title font size, same on ALL cards
   // Fixed row gap used on the center card.  Background cards scale proportionally.
   static const S32 CTR_ROW_GAP  = 16;
   // Approximate available list height on the center card (matches SLOT C geometry: bot=595, top=135).
   static const S32 CTR_LIST_AVAIL = 595 - 6 - (135 + 8 + TITLE_SZ + 4 + 6);  // 595-6-169 = 420

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
      "Weapons",           // 9
      "Mounts",            // 10
   };
   auto phaseTitle = [&](S32 p) -> const char *
   {
      if(p >= 0 && p < TOTAL_PHASES)
         return sPhaseTitles[p];
      return "";
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

   // Weapon phases: draw a slot-selector banner above the list so the player
   // can see which weapon panel this card represents.
   if(phase == PHASE_WEAPONS || phase == PHASE_WEAPON_MOUNT)
   {
      static char slotBuf[128];
      S32 weaponSide = (weaponSideOverride >= 0) ? weaponSideOverride : mWeaponSide;
      S32 slot = MAX(0, MIN(weaponSide, kXtankMaxWeaponSlots - 1));
      XtankWeapon weapon = mDesignInProgress.weapons[slot];
      XtankMountLocation mount = (XtankMountLocation)mDesignInProgress.weaponMounts[slot];

      // On the active center weapons card, show the live highlighted selection
      // (same preview behavior as the right-side vehicle panel) before commit.
      if(phase == PHASE_WEAPONS && isCenter && slot == mWeaponSide)
      {
         if(mHighlightedIndex <= 0)
         {
            weapon = XtankWeaponNone;
            mount = XtankMountNone;
         }
         else if(mHighlightedIndex < mWeaponItems.size())
         {
            weapon = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;
            S32 bodyIdx = MAX(0, MIN((S32)mDesignInProgress.bodyIndex, XtankBodyCount - 1));
            mount = firstValidMount(bodyIdx, weapon, mount);
         }
      }

      const char *mode = (phase == PHASE_WEAPONS) ? "Weapon" : "Mount";
      const char *weaponName = "No Weapon";
      if((S32)weapon >= 0 && (S32)weapon < XtankWeaponCount)
         weaponName = xtankWeaponInfos[(S32)weapon].name;

      dSprintf(slotBuf, sizeof(slotBuf), "\x11  %s #%d / %d : %s (%s)  \x10",
               mode, slot + 1, mSlotCount, weaponName, getMountLabel(mount));
      r.setColor(isCenter ? Colors::cyan : Color(grayLevel, grayLevel, grayLevel));
      drawCenteredString(cx, listTop, ITEM_SZ, slotBuf);
      listTop += ITEM_SZ + 8;  // push the weapon list down past the banner
      r.setColor(isCenter ? Colors::gray40 : Color(grayLevel * 0.6f, grayLevel * 0.6f, grayLevel * 0.6f));
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
      if(!name) 
         name = "";

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
      static char sArmorSidesBuf[6][32];
      if(phase == PHASE_ARMOR_SIDES && i < 6)
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
         drawCenteredString(cx, bot - 14, 10, "Up/Dn=side  +/-=adjust  Tab=next  Esc=cancel");
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
   S32 weaponSideAtSlot[5];
   for(S32 i = 0; i < 5; i++)
   {
      phaseAtSlot[i] = (mPhase + (i - 2) + totalPhases * 10) % totalPhases;
      weaponSideAtSlot[i] = -1;
   }

   // In weapon phase, represent previous/next weapon panels on adjacent cards
   // instead of unrelated phases whenever those panels exist.
   if(mPhase == PHASE_WEAPONS)
   {
      for(S32 i = 0; i < 5; i++)
      {
         S32 side = mWeaponSide + (i - 2);
         if(side >= 0 && side < mSlotCount)
         {
            phaseAtSlot[i] = PHASE_WEAPONS;
            weaponSideAtSlot[i] = side;
         }
      }
   }

   struct CardAnim { S32 phase; S32 weaponSide; S32 fromSlot; S32 toSlot; };
   CardAnim cards[5];

   if(mTransitionTimer.getCurrent() > 0 && mTransitionFromPhase >= 0)
   {
      if(mTransitionForward)
         for(S32 i = 0; i < 5; i++)
         { cards[i] = { phaseAtSlot[i], weaponSideAtSlot[i], MIN(i + 1, 4), i }; }
      else
         for(S32 i = 0; i < 5; i++)
         { cards[i] = { phaseAtSlot[i], weaponSideAtSlot[i], MAX(i - 1, 0), i }; }
   }
   else
      for(S32 i = 0; i < 5; i++)
         cards[i] = { phaseAtSlot[i], weaponSideAtSlot[i], i, i };

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

      renderCard(cx - half_w, top, cx + half_w, bot, ca.phase, cf, ca.weaponSide);
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
   if(mPhase == PHASE_WEAPONS)     return mWeaponItems.size();
   return mWeaponMountItems.size();
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
      for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
      {
         bodyPreview.weapons[i] = preview.weapons[i];
         bodyPreview.weaponMounts[i] = preview.weaponMounts[i];
      }
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
   else if(mPhase == PHASE_WEAPONS)
   {
      S32 slot = mWeaponSide;
      if(slot >= 0 && slot < kXtankMaxWeaponSlots)
      {
         if(mHighlightedIndex <= 0)
         {
            preview.weapons[slot] = XtankWeaponNone;
            preview.weaponMounts[slot] = (S8)XtankMountNone;
         }
         else if(mHighlightedIndex < mWeaponItems.size())
         {
            preview.weapons[slot] = (XtankWeapon)mWeaponItems[mHighlightedIndex].itemIndex;
            XtankMountLocation preferred = (XtankMountLocation)preview.weaponMounts[slot];
            S32 previewBodyIdx = MAX(0, MIN((S32)preview.bodyIndex, XtankBodyCount - 1));
            preview.weaponMounts[slot] = (S8)firstValidMount(previewBodyIdx, preview.weapons[slot], preferred);
         }
      }
   }
   else if(mPhase == PHASE_WEAPON_MOUNT)
   {
      S32 slot = mWeaponSide;
      if(slot >= 0 && slot < kXtankMaxWeaponSlots)
      {
         if(mHighlightedIndex == 0)
            preview.weaponMounts[slot] = (S8)XtankMountNone;
         else if(mHighlightedIndex > 0 && mHighlightedIndex < mWeaponMountItems.size())
            preview.weaponMounts[slot] = (S8)(S32)mWeaponMountItems[mHighlightedIndex].itemIndex;
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

   const XtankEngineInfo &eng = xtankEngineInfos[engineIdx];
   const XtankTreadInfo  &trd = xtankTreadInfos[treadIdx];
   XtankDerivedStats derived = getDerivedStats(preview);

   F32 effSpeed = derived.maxSpeed;
   F32 effRev   = derived.maxReverseSpeed;
   F32 effAccel = derived.maxAccel;
   F32 effTurn  = derived.maxTurnRate;
   S32 fireRatePct = S32((1.0f - xtankHeatSinkFireDelayMult(heatSinks)) * 100.0f + 0.5f);

   S32 totalWeight = body_stat[bodyIdx].weight + eng.weight + heatSinkStat.weight * heatSinks;
   S32 totalCost   = body_stat[bodyIdx].cost   + eng.cost   + trd.cost + heatSinkStat.cost * heatSinks;
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = preview.weapons[i];
      if(w == XtankWeaponNone) continue;
      if((S32)w < 0 || (S32)w >= XtankWeaponCount) continue;
      totalWeight += xtankWeaponInfos[(S32)w].weight;
      totalCost   += xtankWeaponInfos[(S32)w].cost;
   }

   // Armor cost and weight: xtank-style scaling by body size.
   S32 armorIdx = MAX(0, MIN((S32)preview.armorType, XtankArmorCount - 1));
   S32 totalArmorPoints = 0;
   for(S32 i = 0; i < 6; i++)
      totalArmorPoints += preview.armorSides[i];
   totalWeight += totalArmorPoints * xtankArmorInfos[armorIdx].weight * body_stat[bodyIdx].size;
   totalCost   += totalArmorPoints * xtankArmorInfos[armorIdx].cost * body_stat[bodyIdx].size;

   // Suspension, treads, and bumpers scale by body size in xtank.
   totalCost += suspensionStat[MAX(0, MIN((S32)preview.suspensionType, XtankSuspensionCount - 1))].cost * body_stat[bodyIdx].size;
   totalCost += trd.cost * body_stat[bodyIdx].size;
   totalCost += bumperStat[MAX(0, MIN((S32)preview.bumperType, XtankBumperCount - 1))].cost * body_stat[bodyIdx].size;

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
   totalSpace += totalArmorPoints * xtankArmorInfos[armorIdx].space * body_stat[bodyIdx].size;
   totalSpace += heatSinkStat.space * heatSinks;
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
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
   drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Handling: %d  Turrets: %d", body_stat[bodyIdx].handling, xtankTurretInfos[bodyIdx].count);    ty += LINE_GAP;
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

   // Show weapon loadout in two columns so stats remain readable.
   r.setColor(Colors::gray45);
   drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, ty);
   ty += 6;


   ty += renderWeaponList(PNL_LEFT, PNL_CX, ty, preview.weapons, preview.weaponMounts, STAT_SZ - 2, LINE_GAP);

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
   renderCarouselDots(PNL_CX, DOTS_Y);

   r.setColor(Colors::gray50);
   if(mPhase == PHASE_WEAPONS && mSlotCount > 1)
      drawCenteredStringf(PNL_CX, HINT_Y, 11, "[Lt]/[Rt] mount  [Up]/[Dn] weapon  Tab next slot");
   else
      drawCenteredString(PNL_CX, HINT_Y, 11, "[Up]/[Dn] item  Tab/Shift-Tab phase");

   FontManager::popFontContext();
}


S32 UIXtankHelper::renderWeaponList(S32 left, S32 cx, S32 y, const XtankWeapon weapons[], const S8 mounts[], S32 fontSz, S32 lineGap) const
{
   Renderer &r = Renderer::get();

   r.setColor(Colors::gray70);
   drawCenteredString(cx, y, fontSz - 1, "Weapons");
   y += lineGap  ;

   const S32 weaponRows = (kXtankMaxWeaponSlots + 1) / 2;

   // Build all label strings first and measure their widths per column,
   // so we can compute the true block width and center it on the panel.
   char buffers[kXtankMaxWeaponSlots][128];
   S32  colWidth[2] = { 0, 0 };
   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      XtankWeapon w = weapons[i];
      XtankMountLocation mount = (XtankMountLocation)mounts[i];
      const char *mountName = getMountLabel(mount);
      if(w == XtankWeaponNone || (S32)w < 0 || (S32)w >= XtankWeaponCount)
         dSprintf(buffers[i], sizeof(buffers[i]), "#%d %s: --", i + 1, mountName);
      else
         dSprintf(buffers[i], sizeof(buffers[i]), "#%d %s: %s", i + 1, mountName, xtankWeaponInfos[(S32)w].name);

      S32 col = (i < weaponRows) ? 0 : 1;
      colWidth[col] = MAX(colWidth[col], getStringWidth(fontSz, buffers[i]));
   }

   const S32 midMargin  = 14;
   const S32 blockWidth = colWidth[0] + midMargin + colWidth[1];
   const S32 col0X      = cx - blockWidth / 2;
   const S32 col1X      = col0X + colWidth[0] + midMargin;

   for(S32 i = 0; i < kXtankMaxWeaponSlots; i++)
   {
      bool activeSlot = ((mPhase == PHASE_WEAPONS || mPhase == PHASE_WEAPON_MOUNT) && mWeaponSide == i);
      r.setColor(activeSlot ? Colors::overlayMenuSelectedItemColor : Colors::cyan);

      S32 col = (i < weaponRows) ? 0 : 1;
      S32 row = i % weaponRows;
      drawString((col == 0) ? col0X : col1X, y + row * lineGap, fontSz, buffers[i]);
   }
   return (weaponRows + 1) * lineGap;
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
void UIXtankHelper::renderFullBuildStats(S32 cx, S32 y, const XtankDesign &preview) const
{
   static const S32 BSTAT_SZ  = 12;
   static const S32 BSTAT_GAP = BSTAT_SZ + 5;

   XtankDerivedStats derived = getDerivedStats(preview);
   S32 hs = MAX(XtankHeatSinkMin, MIN((S32)preview.heatSinkCount, XtankHeatSinkMax));
   S32 fireRatePct = S32((1.0f - xtankHeatSinkFireDelayMult(hs)) * 100.0f + 0.5f);

   Renderer &r = Renderer::get();

   // Section divider
   const S32 DIVIDER_MARGIN = 80;
   r.setColor(Colors::gray40);
   S32 divY = y - 4;
   drawHorizLine(cx - DIVIDER_MARGIN, cx + DIVIDER_MARGIN, divY);

   // Section label
   r.setColor(Colors::gray50);
   drawCenteredString(cx, divY - BSTAT_SZ - 2, BSTAT_SZ, "CURRENT BUILD");

   // Stat lines
   S32 ty = y + 4;
   r.setColor(Colors::green);
   drawCenteredStringf(cx, ty, BSTAT_SZ, "Spd: %d  Rev: %d", (S32)derived.maxSpeed, (S32)derived.maxReverseSpeed);
   ty += BSTAT_GAP;
   drawCenteredStringf(cx, ty, BSTAT_SZ, "Acc: %d  Turn: %.1f", (S32)derived.maxAccel, derived.maxTurnRate);
   ty += BSTAT_GAP;
   if(fireRatePct > 0)
      drawCenteredStringf(cx, ty, BSTAT_SZ, "Fire rate: +%d%%", fireRatePct);
   else
      drawCenteredString(cx, ty, BSTAT_SZ, "Fire rate: base");
}


#undef UNSEL_COLOR

} /* namespace Zap */
