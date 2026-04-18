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
   if(mPhase == PHASE_BODY)
   {
      updateItemColors(mBodyItems);
      drawItemMenu("Choose vehicle body:",
                   mBodyItems.address(), mBodyItems.size(),
                   NULL, 0,
                   mBodyButtonsWidth, mBodyItemsDisplayWidth);
   }
   else if(mPhase == PHASE_ENGINE)
   {
      updateItemColors(mEngineItems);
      drawItemMenu("Choose engine:",
                   mEngineItems.address(), mEngineItems.size(),
                   mBodyItems.address(),   mBodyItems.size(),
                   mEngineButtonsWidth, mEngineItemsDisplayWidth);
   }
   else if(mPhase == PHASE_TREADS)
   {
      updateItemColors(mTreadItems);
      drawItemMenu("Choose treads:",
                   mTreadItems.address(), mTreadItems.size(),
                   mBodyItems.address(),  mBodyItems.size(),
                   mTreadButtonsWidth, mTreadItemsDisplayWidth);
   }
   else if(mPhase == PHASE_HEATSINK)
   {
      updateItemColors(mHeatSinkItems);
      drawItemMenu("Choose heat sinks:",
                   mHeatSinkItems.address(), mHeatSinkItems.size(),
                   mBodyItems.address(),     mBodyItems.size(),
                   mHeatSinkButtonsWidth, mHeatSinkItemsDisplayWidth);
   }
   else
   {
      // Weapon slot selection: slot indices 0..mSlotCount-1.
      S32 slot = mPhase - PHASE_WEAPONS;   // 0-based slot index
      char title[80];
      dSprintf(title, sizeof(title), "Slot %d of %d — pick weapon:", slot + 1, mSlotCount);

      updateItemColors(mWeaponItems);
      drawItemMenu(title,
                   mWeaponItems.address(), mWeaponItems.size(),
                   mBodyItems.address(),   mBodyItems.size(),
                   mWeaponButtonsWidth, mWeaponItemsDisplayWidth);
   }

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

   // RIGHT arrow: confirm the highlighted item (same as ENTER).
   if(inputCode == KEY_RIGHT)
      inputCode = KEY_ENTER;

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


// Move to the previous phase, restoring the highlighted index from the
// existing design so the player sees their previous choice pre-selected.
void UIXtankHelper::navigateBackward()
{
   if(mPhase <= PHASE_BODY)
      return;  // Already at the first phase

   mPhase--;
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


// Draw a floating preview panel on the right side of the screen.  The panel
// shows a rendered shape (for bodies) or stat text (for all phases) for the
// currently highlighted item, plus a "Combined Build Stats" section that
// reflects the effective vehicle performance as the design evolves.
void UIXtankHelper::renderPreviewPanel() const
{
   // Preview panel geometry (in game canvas coordinates: 1066 × 600, Y down).
   // PNL_TOP moved up and PNL_BOT extended to accommodate the new sections.
   static const S32 PNL_LEFT   = 680;
   static const S32 PNL_RIGHT  = 1050;
   static const S32 PNL_TOP    = 130;
   static const S32 PNL_BOT    = 585;
   static const S32 PNL_CX     = (PNL_LEFT + PNL_RIGHT) / 2;   // 865
   static const S32 CORNER     = 8;

   static const S32 TITLE_SZ   = 16;
   static const S32 STAT_SZ    = 13;
   static const S32 LINE_GAP   = STAT_SZ + 6;

   // Vertical positions for graphic and text elements within the panel.
   // All relative to PNL_TOP for clarity.
   static const S32 TITLE_Y     = PNL_TOP + 10;
   static const S32 GRAPHIC_Y   = PNL_TOP + 160;  // centre of engine/tread graphic
   static const S32 HS_ROW1_Y   = PNL_TOP + 120;  // top row of heat-sink crosses
   static const S32 ENG_TEXT_Y  = PNL_TOP + 210;  // first stat line for engine phase
   static const S32 TRD_TEXT_Y  = PNL_TOP + 215;  // first stat line for tread phase
   static const S32 HS_TEXT_Y   = PNL_TOP + 230;  // stat line for heat-sink phase

   // Armor classification thresholds (body phase).
   static const F32 ARMOR_HEAVY_LIMIT    = 0.70f;
   static const F32 ARMOR_MED_LIMIT      = 0.95f;

   // Y position where the "Combined Build Stats" section begins.
   static const S32 BUILD_SECT_Y = PNL_TOP + 305;
   // Y position for carousel dots.
   static const S32 DOTS_Y       = BUILD_SECT_Y + 95;
   // Y position for the navigation hint text.
   static const S32 HINT_Y       = DOTS_Y + 22;

   // Semi-transparent dark background, red border to match helper menus.
   drawFilledFancyBox(PNL_LEFT, PNL_TOP, PNL_RIGHT, PNL_BOT, CORNER,
                      Colors::black, 0.75f, Colors::red35);

   FontManager::pushFontContext(HelperMenuContext);

   Renderer &r = Renderer::get();

   // -------------------------------------------------------------------------
   // BODY PHASE
   // -------------------------------------------------------------------------
   if(mPhase == PHASE_BODY)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankBodyCount) idx = 0;

      // Title: body name
      r.setColor(Colors::white);
      drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, xtankBodyNames[idx]);

      // Horizontal divider
      r.setColor(Colors::gray40);
      S32 divY = TITLE_Y + TITLE_SZ + 4;
      drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, divY);

      // Render the vehicle body shape, centered in the upper portion of the
      // panel.  Scale so the largest bodies (~44 units) fit within ~130 px.
      // Use a fixed display scale that looks good across all 14 bodies.
      static const F32 BODY_SCALE = 2.8f;
      static const S32 BODY_CY    = PNL_TOP + 150;  // vertical centre for body graphic

      // Thrusts: [forward, reverse, port, starboard] — all zero for a static preview.
      static const F32 thrusts[4] = { 0, 0, 0, 0 };

      r.pushMatrix();
      r.translate(F32(PNL_CX), F32(BODY_CY), 0);
      r.scale(BODY_SCALE);
      // Rotate so the nose points up on screen.  In the xtank vertex space the
      // nose is at +Y; with Y-down screen coords that puts the nose at the
      // bottom.  A 180° rotation flips it so the nose points up.
      r.rotate(180.0f, 0, 0, 1.0f);

      const ShipShapeInfo* shapeInfo = &xtankBodyInfos[idx];
      const Color* shipColor = &Colors::blue;
      renderShip(shapeInfo, shipColor, Colors::blue, 1.0f,
                 const_cast<F32*>(thrusts), 1.0f, F32(Ship::CollisionRadius), 0,
                 false, false, false, false);

      // Draw turrets pointing straight up
      renderXtankTurrets(Point(0, 0), 0, 0, 1.0f,
                         xtankTurretInfos[idx], &Colors::blue, 1.0f);

      r.popMatrix();

      // Stats text
      const TankPhysicsInfo &phy = xtankPhysicsInfos[idx];
      S32 turrets = xtankTurretInfos[idx].count;

      const char *armClass = (phy.armor <= ARMOR_HEAVY_LIMIT) ? "Heavy" :
                             (phy.armor <= ARMOR_MED_LIMIT)   ? "Med"   : "Light";

      S32 ty = BODY_CY + S32(xtankBodyCollisionRadius[idx] * BODY_SCALE) + 16;

      r.setColor(Colors::cyan);
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Speed: %d  Rev: %d", (S32)phy.maxSpeed, (S32)phy.maxReverseSpeed);
      ty += LINE_GAP;
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Accel: %d  Friction: %d", (S32)phy.acceleration, (S32)phy.friction);
      ty += LINE_GAP;
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Turn: %.1f rad/s", phy.turnRate);
      ty += LINE_GAP;
      r.setColor(Colors::yellow);
      drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Armor: %s  Turrets: %d", armClass, turrets);

      // Body phase: combined stats use the highlighted body with the existing
      // engine/tread/heatsink choices (or defaults if not yet set).
      renderFullBuildStats(PNL_CX, BUILD_SECT_Y,
                           idx,
                           (S32)mDesignInProgress.engineType,
                           (S32)mDesignInProgress.treadType,
                           (S32)mDesignInProgress.heatSinkCount);
   }

   // -------------------------------------------------------------------------
   // ENGINE PHASE
   // -------------------------------------------------------------------------
   else if(mPhase == PHASE_ENGINE)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankEngineCount) idx = 0;

      const XtankEngineInfo &ei = xtankEngineInfos[idx];

      r.setColor(Colors::white);
      drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, ei.name);

      r.setColor(Colors::gray40);
      S32 divY = TITLE_Y + TITLE_SZ + 4;
      drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, divY);

      // Draw the engine diamond symbol, coloured to match the in-game overlay.
      static const F32    DIAM_SZ         = 30.0f;
      // Colors mirror those used in renderXtankVehicleOverlay() in gameObjectRender.cpp.
      static const Color  ENGINE_LIGHT_COLOR(0.3f, 0.4f, 1.0f);    // dim blue
      static const Color  ENGINE_STD_COLOR  (0.95f, 0.35f, 0.05f); // orange-red
      static const Color  ENGINE_HEAVY_COLOR(1.0f, 0.65f, 0.0f);   // bright orange
      Color engColor;
      switch((XtankEngine)idx)
      {
         case XtankEngine::Small_Electric:
         case XtankEngine::Small_Combustion:
         case XtankEngine::Small_Turbine:
            engColor = ENGINE_LIGHT_COLOR;
            break;

         case XtankEngine::Medium_Electric:
         case XtankEngine::Medium_Combustion:
         case XtankEngine::Medium_Turbine:
         case XtankEngine::Fuel_Cell:
            engColor = ENGINE_STD_COLOR;
            break;

         default:
            engColor = ENGINE_HEAVY_COLOR;
            break;
      }
      F32 gy = F32(GRAPHIC_Y);
      F32 diam[] = {
          F32(PNL_CX),           gy - DIAM_SZ,
          F32(PNL_CX) + DIAM_SZ, gy,
          F32(PNL_CX),           gy + DIAM_SZ,
          F32(PNL_CX) - DIAM_SZ, gy,
          F32(PNL_CX),           gy - DIAM_SZ,
      };
      r.setColor(engColor);
      r.renderVertexArray(diam, 5, RenderType::LineStrip);

      S32 ty = ENG_TEXT_Y;
      r.setColor(Colors::cyan);
      S32 spdPct = S32((ei.speedMult - 1.0f) * 100.0f + 0.5f);
      S32 accPct = S32((ei.accelMult - 1.0f) * 100.0f + 0.5f);
      drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                          "Top speed:    %+d%%", spdPct);
      ty += LINE_GAP;
      drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                          "Acceleration: %+d%%", accPct);

      // Flavour text
      static const char *engDesc[XtankEngineCount] = {
         "Lighter plant, reduced power",
         "Balanced performance",
         "Heavy plant, maximum power",
      };
      ty += LINE_GAP * 2;
      if(idx < XtankEngineCount && engDesc[idx] != nullptr)
      {
         r.setColor(Colors::gray70);
         drawCenteredString(PNL_CX, ty, STAT_SZ, engDesc[idx]);
      }

      renderFullBuildStats(PNL_CX, BUILD_SECT_Y,
                           (S32)mDesignInProgress.bodyIndex,
                           idx,
                           (S32)mDesignInProgress.treadType,
                           (S32)mDesignInProgress.heatSinkCount);
   }

   // -------------------------------------------------------------------------
   // TREADS PHASE
   // -------------------------------------------------------------------------
   else if(mPhase == PHASE_TREADS)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankTreadCount) idx = 0;

      const XtankTreadInfo &ti = xtankTreadInfos[idx];

      r.setColor(Colors::white);
      drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, ti.name);

      r.setColor(Colors::gray40);
      S32 divY = TITLE_Y + TITLE_SZ + 4;
      drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, divY);

      // Simple tread-track graphic: two parallel rectangles.
      static const F32 TR_W   = 12.0f;
      static const F32 TR_H   = 60.0f;
      static const F32 TR_SEP = 28.0f;  // distance from centre to each track

      F32 trCy = F32(GRAPHIC_Y);
      Color trColor = Colors::green65;
      r.setColor(trColor);
      // Left track
      drawHollowRect(PNL_CX - TR_SEP - TR_W, trCy - TR_H * 0.5f,
                     PNL_CX - TR_SEP + TR_W, trCy + TR_H * 0.5f);
      // Right track
      drawHollowRect(PNL_CX + TR_SEP - TR_W, trCy - TR_H * 0.5f,
                     PNL_CX + TR_SEP + TR_W, trCy + TR_H * 0.5f);

      S32 ty = TRD_TEXT_Y;
      r.setColor(Colors::cyan);
      S32 turnPct = S32((ti.friction - 1.0f) * 100.0f + 0.5f);
      S32 fricPct = S32((ti.friction - 1.0f) * 100.0f + 0.5f);
      drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                          "Turn rate:   %+d%%", turnPct);
      ty += LINE_GAP;
      drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                          "Braking:     %+d%%", fricPct);

      static const char *trdDesc[XtankTreadCount] = {
         "Nimble steering, less grip",
         "Balanced handling",
         "Slower turns, heavy braking",
      };
      ty += LINE_GAP * 2;
      if(idx < XtankTreadCount && trdDesc[idx] != nullptr)
      { 
         r.setColor(Colors::gray70);
         drawCenteredString(PNL_CX, ty, STAT_SZ, trdDesc[idx]);
      }

      renderFullBuildStats(PNL_CX, BUILD_SECT_Y,
                           (S32)mDesignInProgress.bodyIndex,
                           (S32)mDesignInProgress.engineType,
                           idx,
                           (S32)mDesignInProgress.heatSinkCount);
   }

   // -------------------------------------------------------------------------
   // HEAT SINK PHASE
   // -------------------------------------------------------------------------
   else if(mPhase == PHASE_HEATSINK)
   {
      S32 idx  = mHighlightedIndex;
      S32 n    = idx + XtankHeatSinkMin;   // actual sink count (1-6)
      if(n < XtankHeatSinkMin) n = XtankHeatSinkMin;
      if(n > XtankHeatSinkMax) n = XtankHeatSinkMax;

      char titleBuf[32];
      dSprintf(titleBuf, sizeof(titleBuf), "Heat Sinks: %d", n);
      r.setColor(Colors::white);
      drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, titleBuf);

      r.setColor(Colors::gray40);
      S32 divY = TITLE_Y + TITLE_SZ + 4;
      drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, divY);

      // Draw N small cyan cross (+) symbols arranged in a 3-column grid.
      static const F32 HS_SPACING = 40.0f;
      static const F32 HS_ARM     = 10.0f;
      r.setColor(Colors::cyan);
      for(S32 i = 0; i < n; i++)
      {
         F32 col = F32(i % 3);
         F32 row = F32(i / 3);
         F32 cx  = F32(PNL_CX) - HS_SPACING + col * HS_SPACING;
         F32 cy  = F32(HS_ROW1_Y) + row * HS_SPACING;
         F32 h[] = { cx - HS_ARM, cy,   cx + HS_ARM, cy };
         F32 v[] = { cx, cy - HS_ARM,   cx, cy + HS_ARM };
         r.renderVertexArray(h, 2, RenderType::Lines);
         r.renderVertexArray(v, 2, RenderType::Lines);
      }

      F32 mult = xtankHeatSinkFireDelayMult(n);
      S32 pct  = S32((1.0f - mult) * 100.0f + 0.5f);

      S32 ty = HS_TEXT_Y;
      r.setColor(Colors::cyan);
      if(pct == 0)
         drawCenteredString(PNL_CX, ty, STAT_SZ, "No fire-rate bonus");
      else
         drawCenteredStringf(PNL_CX, ty, STAT_SZ, "Fire-rate: +%d%%", pct);

      ty += LINE_GAP;
      r.setColor(Colors::gray70);
      drawCenteredString(PNL_CX, ty, STAT_SZ, "More sinks = faster cycling");

      renderFullBuildStats(PNL_CX, BUILD_SECT_Y,
                           (S32)mDesignInProgress.bodyIndex,
                           (S32)mDesignInProgress.engineType,
                           (S32)mDesignInProgress.treadType,
                           n);
   }

   // -------------------------------------------------------------------------
   // WEAPON PHASE
   // -------------------------------------------------------------------------
   else
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= mWeaponItems.size()) idx = 0;

      r.setColor(Colors::white);

      if(idx == 0)
      {
         // "None" option
         drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, "None");
         r.setColor(Colors::gray40);
         drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, TITLE_Y + TITLE_SZ + 4);
         r.setColor(Colors::gray67);
         drawCenteredString(PNL_CX, HS_ROW1_Y, STAT_SZ, "Empty turret slot");
      }
      else
      {
         S32 w = idx - 1;  // 0-based weapon index
         if(w < 0 || w >= XtankWeaponCount) w = 0;
         const XtankWeaponInfo &wi = xtankWeaponInfos[w];

         drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, wi.name);
         r.setColor(Colors::gray40);
         drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, TITLE_Y + TITLE_SZ + 4);

         S32 ty = TITLE_Y + TITLE_SZ + 4 + 20;
         r.setColor(Colors::cyan);
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Fire delay:  %d ms", (S32)xtankFireDelayMs(wi));
         ty += LINE_GAP;
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Energy:      %d/shot", (S32)wi.heat);
         ty += LINE_GAP;
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Speed:       %d u/s", (S32)xtankProjVelocity(wi));
         ty += LINE_GAP;
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Lifetime:    %d ms", (S32)xtankProjLiveTime(wi));
      }

      // Weapon phase: mobility stats reflect the completed chassis choices.
      renderFullBuildStats(PNL_CX, BUILD_SECT_Y,
                           (S32)mDesignInProgress.bodyIndex,
                           (S32)mDesignInProgress.engineType,
                           (S32)mDesignInProgress.treadType,
                           (S32)mDesignInProgress.heatSinkCount);
   }

   // Carousel position indicator and navigation hints.
   renderCarouselDots(PNL_CX, DOTS_Y);

   r.setColor(Colors::gray50);
   drawCenteredString(PNL_CX, HINT_Y, 11, "[Up]/[Dn] preview  [Lt]/[Rt] navigate");

   FontManager::popFontContext();
}


// Draw a row of small filled/hollow circles indicating the current carousel
// position.  Past phases are half-bright; future phases are dim; the current
// phase is bright white.  < and > arrow glyphs flank the dots and are grayed
// out when at the first or last phase respectively.
void UIXtankHelper::renderCarouselDots(S32 cx, S32 y) const
{
   // Total number of phases including all weapon slots.
   // Before the body is selected mSlotCount may be 0; show at least 4 dots.
   S32 totalPhases = PHASE_WEAPONS + (mSlotCount > 0 ? mSlotCount : 4);

   static const F32 DOT_R      = 4.0f;   // radius of the current-phase dot
   static const F32 DOT_R_SM   = 3.0f;   // radius of past/future dots
   static const F32 DOT_STEP   = 14.0f;  // horizontal spacing between dot centres
   static const S32 ARROW_GAP  = 12;     // gap from outermost dot to arrow glyph
   static const S32 ARROW_SZ   = 11;     // font size for < > glyphs

   // Centre the dots row at cx.
   F32 rowWidth   = F32(totalPhases - 1) * DOT_STEP;
   F32 dotsLeft   = F32(cx) - rowWidth * 0.5f;

   Renderer &r = Renderer::get();
   static const S32 SEG = 12;  // polygon segments for each circle

   for(S32 i = 0; i < totalPhases; i++)
   {
      F32 dx  = dotsLeft + F32(i) * DOT_STEP;
      F32 rad = (i == mPhase) ? DOT_R : DOT_R_SM;
      bool filled = (i <= mPhase);

      // Colour: current = white, past = gray67, future = gray40.
      if(i == mPhase)
         r.setColor(Colors::white);
      else if(i < mPhase)
         r.setColor(Colors::gray67);
      else
         r.setColor(Colors::gray40);

      if(filled)
      {
         // Filled circle via TriangleFan (center + SEG edge + 1 closing vertex).
         F32 verts[2 + (SEG + 1) * 2];
         verts[0] = dx;
         verts[1] = F32(y);
         // Loop s from 0..SEG: the extra s==SEG iteration wraps via % SEG to
         // repeat the first edge vertex, closing the filled circle.
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
         // Hollow circle via LineLoop.
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

   // Left arrow < — grayed out when at first phase.
   S32 arrowY = y - ARROW_SZ / 2;
   S32 leftArrowX = S32(dotsLeft) - ARROW_GAP - ARROW_SZ;
   r.setColor((mPhase > PHASE_BODY) ? Colors::gray67 : Colors::gray20);
   drawCenteredString(leftArrowX, arrowY, ARROW_SZ, "<");

   // Right arrow > — grayed out when at last phase.
   S32 rightArrowX = S32(dotsLeft + rowWidth) + ARROW_GAP;
   r.setColor((mPhase < totalPhases - 1) ? Colors::gray67 : Colors::gray20);
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
