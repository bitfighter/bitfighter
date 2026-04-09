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
      // Weapon slot selection: slot indices 0..mSlotCount-1.
      S32 slot = mPhase - PHASE_WEAPONS;   // 0-based slot index
      char title[80];
      dSprintf(title, sizeof(title), "Slot %d of %d — pick weapon:", slot + 1, mSlotCount);

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
            mHighlightedIndex = 0;

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
            mHighlightedIndex = 0;

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
            mHighlightedIndex = 0;

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
            mHighlightedIndex = 0;
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
         mHighlightedIndex = 0;
         advanceToNextPhaseOrFinish();
         return true;
      }

      for(S32 w = 0; w < XtankWeapon::Count; w++)
      {
         if(inputCode == sWeaponKeys[w + 1])
         {
            mDesignInProgress.weapons[slot] = (XtankWeapon::Type)w;
            mHighlightedIndex = 0;
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
   mHighlightedIndex = 0;
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
// currently highlighted item, letting the player make an informed choice
// before committing to a key press.
void UIXtankHelper::renderPreviewPanel() const
{
   // Preview panel geometry (in game canvas coordinates: 1066 × 600, Y down).
   static const S32 PNL_LEFT   = 680;
   static const S32 PNL_RIGHT  = 1050;
   static const S32 PNL_TOP    = 155;
   static const S32 PNL_BOT    = 530;
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
      if(idx < 0 || idx >= XtankBody::Count) idx = 0;

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


      //// Overload that accepts a ShipShapeInfo directly (used when rendering xtank bodies)
      // extern void renderShip(const ShipShapeInfo * shapeInfo, const Color * shipColor, const Color & hbc, F32 alpha, F32 thrusts[], F32 health, F32 radius, U32 sensorTime,
      //    bool shieldActive, bool sensorActive, bool repairActive, bool hasArmor);

      const ShipShapeInfo* shapeInfo = &xtankBodyInfos[idx];
      const Color* shipColor = &Colors::blue;
      renderShip(shapeInfo, shipColor, Colors::blue, 1.0f,
                 const_cast<F32*>(thrusts), 1.0f, F32(Ship::CollisionRadius), 0,
                 false, false, false, false);


      // Draw turrets pointing straight up (aim = bodyAngle + Pi/2, but for a
      // static preview just aim upward: since we've already rotated 180°, pass
      // 0 for both bodyAngle and aimAngle — turrets will point in +Y of the
      // rotated frame which is toward the top of the screen).
      renderXtankTurrets(Point(0, 0), 0, 0, 1.0f,
                         xtankTurretInfos[idx], &Colors::blue, 1.0f);

      r.popMatrix();

      // Stats text
      const TankPhysicsInfo &phy = xtankPhysicsInfos[idx];
      S32 turrets = xtankTurretInfos[idx].count;

      // Armor value < 1.0 means reduced damage taken (i.e. stronger/heavier armour).
      // "Heavy" here denotes the most protective armour (lowest numeric value).
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
   }

   // -------------------------------------------------------------------------
   // ENGINE PHASE
   // -------------------------------------------------------------------------
   else if(mPhase == PHASE_ENGINE)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankEngine::Count) idx = 0;

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
      switch((XtankEngine::Type)idx)
      {
         case XtankEngine::Light:   engColor = ENGINE_LIGHT_COLOR; break;
         case XtankEngine::Heavy:   engColor = ENGINE_HEAVY_COLOR; break;
         default:                   engColor = ENGINE_STD_COLOR;   break;
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
      static const char *engDesc[XtankEngine::Count] = {
         "Lighter plant, reduced power",
         "Balanced performance",
         "Heavy plant, maximum power",
      };
      ty += LINE_GAP * 2;
      r.setColor(Colors::gray70);
      drawCenteredString(PNL_CX, ty, STAT_SZ, engDesc[idx]);
   }

   // -------------------------------------------------------------------------
   // TREADS PHASE
   // -------------------------------------------------------------------------
   else if(mPhase == PHASE_TREADS)
   {
      S32 idx = mHighlightedIndex;
      if(idx < 0 || idx >= XtankTread::Count) idx = 0;

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
      Color trColor = (idx == (S32)XtankTread::Rubber) ? Colors::green65 :
                      (idx == (S32)XtankTread::Metal)  ? Colors::gray67  : Colors::orange67;
      r.setColor(trColor);
      // Left track
      drawHollowRect(PNL_CX - TR_SEP - TR_W, trCy - TR_H * 0.5f,
                     PNL_CX - TR_SEP + TR_W, trCy + TR_H * 0.5f);
      // Right track
      drawHollowRect(PNL_CX + TR_SEP - TR_W, trCy - TR_H * 0.5f,
                     PNL_CX + TR_SEP + TR_W, trCy + TR_H * 0.5f);

      S32 ty = TRD_TEXT_Y;
      r.setColor(Colors::cyan);
      S32 turnPct = S32((ti.turnMult - 1.0f) * 100.0f + 0.5f);
      S32 fricPct = S32((ti.frictionMult - 1.0f) * 100.0f + 0.5f);
      drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                          "Turn rate:   %+d%%", turnPct);
      ty += LINE_GAP;
      drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                          "Braking:     %+d%%", fricPct);

      static const char *trdDesc[XtankTread::Count] = {
         "Nimble steering, less grip",
         "Balanced handling",
         "Slower turns, heavy braking",
      };
      ty += LINE_GAP * 2;
      r.setColor(Colors::gray70);
      drawCenteredString(PNL_CX, ty, STAT_SZ, trdDesc[idx]);
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
         if(w < 0 || w >= XtankWeapon::Count) w = 0;
         const XtankWeaponInfo &wi = xtankWeaponInfos[w];

         drawCenteredString(PNL_CX, TITLE_Y, TITLE_SZ, wi.name);
         r.setColor(Colors::gray40);
         drawHorizLine(PNL_LEFT + 10, PNL_RIGHT - 10, TITLE_Y + TITLE_SZ + 4);

         S32 ty = TITLE_Y + TITLE_SZ + 4 + 20;
         r.setColor(Colors::cyan);
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Fire delay:  %d ms", (S32)wi.fireDelay);
         ty += LINE_GAP;
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Energy:      %d/shot", (S32)wi.energyDrain);
         ty += LINE_GAP;
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Speed:       %d u/s", (S32)wi.projVelocity);
         ty += LINE_GAP;
         drawCenteredStringf(PNL_CX, ty, STAT_SZ,
                             "Lifetime:    %d ms", (S32)wi.projLiveTime);
      }
   }

   // Arrow key hint at the bottom of the panel
   r.setColor(Colors::gray50);
   drawCenteredString(PNL_CX, PNL_BOT - 16, 11, "[Up]/[Dn] to cycle preview");

   FontManager::popFontContext();
}


#undef UNSEL_COLOR

} /* namespace Zap */
