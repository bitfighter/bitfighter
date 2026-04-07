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

#include "stringUtils.h"

#ifdef TNL_OS_WIN32
#  include <windows.h>     // For ARRAYSIZE
#endif

namespace Zap
{

// For clarity and consistency with other helpers
#define UNSEL_COLOR &Colors::overlayMenuUnselectedItemColor


// Keys used for the 14 bodies: 1-9, 0, A-D.
static const InputCode sBodyKeys[XtankBody::Count] =
{
   KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7,
   KEY_8, KEY_9, KEY_0, KEY_A, KEY_B, KEY_C, KEY_D,
};

// Keys used for engine/tread selections (3 options each, keys 1-3).
static const InputCode s3Keys[3] =
{
   KEY_1, KEY_2, KEY_3,
};

// Keys used for heat-sink count (6 options, keys 1-6).
static const InputCode sHeatSinkKeys[XtankHeatSinkMax] =
{
   KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6,
};

// Keys used for the 10 weapon options (None = 0, weapons 1-9).
// None is placed at KEY_0 so number keys 1-9 map directly to weapon indices.
static const InputCode sWeaponKeys[XtankWeapon::Count + 1] =
{
   KEY_0,     // None
   KEY_1,     // MachineGun
   KEY_2,     // Laser
   KEY_3,     // Missile
   KEY_4,     // Grenade
   KEY_5,     // Rocket
   KEY_6,     // Acid
   KEY_7,     // Tracer
   KEY_8,     // Bomb
   KEY_9,     // Fire
};

// Phase constants (must match those in UIXtankHelper.h)
const S32 UIXtankHelper::PHASE_BODY;
const S32 UIXtankHelper::PHASE_ENGINE;
const S32 UIXtankHelper::PHASE_TREADS;
const S32 UIXtankHelper::PHASE_HEATSINK;
const S32 UIXtankHelper::PHASE_WEAPONS;


////////////////////////////////////////
////////////////////////////////////////

// Constructor
UIXtankHelper::UIXtankHelper()
{
   mPhase      = 0;
   mSlotCount  = 0;
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
   for(S32 i = 0; i < XtankBody::Count; i++)
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
      static string helpTexts[XtankBody::Count];
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
   static const char *engineHelp[XtankEngine::Count] =
   {
      "Slower top speed and acceleration",
      "Balanced performance",
      "Higher top speed and acceleration",
   };

   for(S32 e = 0; e < XtankEngine::Count; e++)
   {
      OverlayMenuItem item;
      item.key                 = s3Keys[e];
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)e;
      item.name                = xtankEngineInfos[e].name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = engineHelp[e];
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mEngineItems.push_back(item);
   }
}


// Build the tread-selection menu items.
void UIXtankHelper::buildTreadItems()
{
   mTreadItems.clear();
   static const char *treadHelp[XtankTread::Count] =
   {
      "Nimble steering, slightly less traction",
      "Balanced handling",
      "Slower turning, higher grip and braking",
   };

   for(S32 t = 0; t < XtankTread::Count; t++)
   {
      OverlayMenuItem item;
      item.key                 = s3Keys[t];
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)t;
      item.name                = xtankTreadInfos[t].name;
      item.itemColor           = UNSEL_COLOR;
      item.help                = treadHelp[t];
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
      item.key                 = sWeaponKeys[0];
      item.button              = KEY_NONE;
      item.showOnMenu          = true;
      item.itemIndex           = (U32)XtankWeapon::None + 1;  // offset so 0 = None stored safely
      item.name                = "None";
      item.itemColor           = UNSEL_COLOR;
      item.help                = "Empty slot";
      item.helpColor           = UNSEL_COLOR;
      item.buttonOverrideColor = NULL;
      mWeaponItems.push_back(item);
   }

   // One entry per weapon.
   for(S32 w = 0; w < XtankWeapon::Count; w++)
   {
      OverlayMenuItem item;
      item.key                 = sWeaponKeys[w + 1];
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


void UIXtankHelper::onActivated()
{
   mPhase = 0;

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
   Parent::onActivated();
}


void UIXtankHelper::render()
{
   if(mPhase == PHASE_BODY)
   {
      drawItemMenu("Choose vehicle body:",
                   mBodyItems.address(), mBodyItems.size(),
                   NULL, 0,
                   mBodyButtonsWidth, mBodyItemsDisplayWidth);
   }
   else if(mPhase == PHASE_ENGINE)
   {
      drawItemMenu("Choose engine:",
                   mEngineItems.address(), mEngineItems.size(),
                   mBodyItems.address(),   mBodyItems.size(),
                   mEngineButtonsWidth, mEngineItemsDisplayWidth);
   }
   else if(mPhase == PHASE_TREADS)
   {
      drawItemMenu("Choose treads:",
                   mTreadItems.address(), mTreadItems.size(),
                   mBodyItems.address(),  mBodyItems.size(),
                   mTreadButtonsWidth, mTreadItemsDisplayWidth);
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      drawItemMenu("Choose heat sinks:",
                   mHeatSinkItems.address(), mHeatSinkItems.size(),
                   mBodyItems.address(),     mBodyItems.size(),
                   mHeatSinkButtonsWidth, mHeatSinkItemsDisplayWidth);
   }
   else
   {
      // Weapon slot selection: slot (mPhase - PHASE_WEAPONS).
      S32 slot = mPhase - PHASE_WEAPONS + 1;
      char title[80];
      dSprintf(title, sizeof(title), "Slot %d of %d — pick weapon:", slot, mSlotCount);

      drawItemMenu(title,
                   mWeaponItems.address(), mWeaponItems.size(),
                   mBodyItems.address(),   mBodyItems.size(),
                   mWeaponButtonsWidth, mWeaponItemsDisplayWidth);
   }
}


// Return true if the key was handled.
bool UIXtankHelper::processInputCode(InputCode inputCode)
{
   if(Parent::processInputCode(inputCode))   // Check for cancel keys
      return true;

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
            return true;
         }
      }
   }
   else if(mPhase == PHASE_ENGINE)
   {
      // --- Engine selection ---
      for(S32 e = 0; e < XtankEngine::Count; e++)
      {
         if(inputCode == s3Keys[e])
         {
            mDesignInProgress.engineType = (XtankEngine::Type)e;
            mPhase = PHASE_TREADS;

            S32 newWidth = getTotalDisplayWidth(mTreadButtonsWidth, mTreadItemsDisplayWidth);
            setExpectedWidth_MidTransition(newWidth);
            resetScrollTimer();
            return true;
         }
      }
   }
   else if(mPhase == PHASE_TREADS)
   {
      // --- Tread selection ---
      for(S32 t = 0; t < XtankTread::Count; t++)
      {
         if(inputCode == s3Keys[t])
         {
            mDesignInProgress.treadType = (XtankTread::Type)t;
            mPhase = PHASE_HEATSINK;

            S32 newWidth = getTotalDisplayWidth(mHeatSinkButtonsWidth, mHeatSinkItemsDisplayWidth);
            setExpectedWidth_MidTransition(newWidth);
            resetScrollTimer();
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

      // None (key 0)
      if(inputCode == sWeaponKeys[0])
      {
         mDesignInProgress.weapons[slot] = XtankWeapon::None;
         advanceToNextPhaseOrFinish();
         return true;
      }

      for(S32 w = 0; w < XtankWeapon::Count; w++)
      {
         if(inputCode == sWeaponKeys[w + 1])
         {
            mDesignInProgress.weapons[slot] = (XtankWeapon::Type)w;
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
   }
   else
   {
      resetScrollTimer();
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


#undef UNSEL_COLOR

} /* namespace Zap */
