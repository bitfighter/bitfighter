//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LoadoutIndicator.h"

#include "ClientGame.h"
#include "FontManager.h"

#include "Colors.h"

#include "Renderer.h"
#include "RenderUtils.h"
#include "stringUtils.h"
#include "VehicleDesign.h"     // For xtankBodyNames, XtankWeapon


using namespace Zap;

namespace Zap { namespace UI {

static const char *getMountAbbrev(XtankMountLocation mount)
{
   switch(mount)
   {
      case XtankMountLocation::TURRET1: return "T1";
      case XtankMountLocation::TURRET2: return "T2";
      case XtankMountLocation::TURRET3: return "T3";
      case XtankMountLocation::TURRET4: return "T4";
      case XtankMountLocation::FRONT:   return "F";
      case XtankMountLocation::BACK:    return "B";
      case XtankMountLocation::LEFT:    return "L";
      case XtankMountLocation::RIGHT:   return "R";
      default:            return "-";
   }
}


// Constructor
LoadoutIndicator::LoadoutIndicator()
{
   mScrollTimer.setPeriod(200);     // Transition time between old and new loadout after loadout changes
}


// Destructor
LoadoutIndicator::~LoadoutIndicator()
{
   // Do nothing
}


void LoadoutIndicator::reset()
{
   mCurrLoadout.reset();
   mPrevLoadout.reset();
   mXtankDesign = VehicleDesign();   // Reset to bodyIndex=-1 (no xtank)
   resetScrollTimer();
}


void LoadoutIndicator::newLoadoutHasArrived(const LoadoutTracker &loadout)
{
   mPrevLoadout.update(mCurrLoadout);
   bool loadoutChanged = mCurrLoadout.update(loadout);

   if(loadoutChanged)
   {
      onActivated();
      resetScrollTimer();
   }
}


void LoadoutIndicator::setActiveWeapon(U32 weaponIndex)
{
   mCurrLoadout.setActiveWeapon(weaponIndex);
}


void LoadoutIndicator::setModulePrimary(ShipModule module, bool isActive)
{
   mCurrLoadout.setModulePrimary(module, isActive);
}


void LoadoutIndicator::setModuleSecondary(ShipModule module, bool isActive)
{
    mCurrLoadout.setModuleSecondary(module, isActive);
}


void LoadoutIndicator::setXtankDesign(const VehicleDesign &design)
{
   mXtankDesign = design;
}


const LoadoutTracker *LoadoutIndicator::getLoadout() const
{
   return &mCurrLoadout;
}


static const S32 IndicatorHeight = IndicatorFontSize + 2 * IndicatorVertPadding + 1;


static S32 getComponentRectWidth(S32 textWidth)
{
   return textWidth + 2 * IndicatorHorizPadding;
}


// Returns width of indicator component
S32 renderComponentIndicator(S32 xPos, S32 yPos, const char *name)
{
   // Draw the weapon or module name (n.b.: If you change the lcase, do the same in getComponentIndicatorWidth)
   S32 textWidth = drawStringAndGetWidth(xPos + IndicatorHorizPadding, yPos + IndicatorVertPadding - 1,
                                         IndicatorFontSize, lcase(name).c_str());

   S32 rectWidth = getComponentRectWidth(textWidth);

   drawFancyBox(xPos, yPos, xPos + rectWidth, yPos + IndicatorHeight, IndicatorVertPadding, RenderType::LineLoop);

   return rectWidth;
}


S32 getComponentIndicatorWidth(const char *name)
{
   return getComponentRectWidth(getStringWidth(IndicatorFontSize, lcase(name).c_str()));
}


static const S32 GapBetweenTheGroups = 20;

static const Color *INDICATOR_INACTIVE_COLOR = &Colors::green80;
static const Color *INDICATOR_ACTIVE_COLOR = &Colors::red80;
static const Color *INDICATOR_PASSIVE_COLOR = &Colors::yellow;


void setModuleColor(Renderer &r, const ShipModule &module, bool isPrimaryActive, bool isSecondaryActive)
{
   if (gModuleInfo[module].getPrimaryUseType() == ModulePrimaryUsePassive ||   // Armor
      gModuleInfo[module].getPrimaryUseType() == ModulePrimaryUseHybrid)      // Sensor
   {
      r.setColor(*INDICATOR_PASSIVE_COLOR);
   }
   else if (isPrimaryActive)
      r.setColor(*INDICATOR_ACTIVE_COLOR);
   else
      r.setColor(*INDICATOR_INACTIVE_COLOR);

   // Always change to orange if module secondary is fired
   if (gModuleInfo[module].hasSecondary() && isSecondaryActive)
      r.setColor(Colors::orange67);
}


// Returns width
static S32 doRender(const LoadoutTracker &loadout, const VehicleDesign &xtankDesign,
                    ClientGame *game, S32 top)
{
   Renderer& r = Renderer::get();

   // Xtank mode: show body name + engine + treads + heat sinks + per-slot weapon names.
   if(xtankDesign.isXtankVehicle())
   {
      FontManager::pushFontContext(LoadoutIndicatorContext);

      S32 xPos = LoadoutIndicator::LoadoutIndicatorLeftPos;
      S32 bodyIdx = (S32)xtankDesign.body;

      // Body name box
      r.setColor(*INDICATOR_INACTIVE_COLOR);
      S32 width = renderComponentIndicator(xPos, top, xtankBodyNames[bodyIdx]);
      xPos += width + IndicatorHorizPadding;

      xPos += GapBetweenTheGroups;

      // Engine
      r.setColor(*INDICATOR_INACTIVE_COLOR);
      width = renderComponentIndicator(xPos, top, xtankEngineInfos[(S32)xtankDesign.engine].name);
      xPos += width + IndicatorHorizPadding;

      // Treads
      r.setColor(*INDICATOR_INACTIVE_COLOR);
      width = renderComponentIndicator(xPos, top, xtankTreadInfos[(S32)xtankDesign.tread].name);
      xPos += width + IndicatorHorizPadding;

      // Heat sinks ("HSx N")
      char hsBuf[16];
      dSprintf(hsBuf, sizeof(hsBuf), "HS: %d", (S32)xtankDesign.heatSinks);
      r.setColor(*INDICATOR_INACTIVE_COLOR);
      width = renderComponentIndicator(xPos, top, hsBuf);
      xPos += width + IndicatorHorizPadding;

      xPos += GapBetweenTheGroups;

      // One box per xtank weapon-number slot
      for(S32 i = 0; i < WEAPON_SLOTS; i++)
      {
         XtankWeapon wt = xtankDesign.weapons[i];
         char weapBuf[96];
         if(wt != XtankWeapon::NONE)
            dSprintf(weapBuf, sizeof(weapBuf), "#%d %s:%s", i + 1, getMountAbbrev(xtankDesign.weaponMounts[i]), xtankWeaponInfos[(S32)wt].name);
         else
            dSprintf(weapBuf, sizeof(weapBuf), "#%d --", i + 1);
         r.setColor(*INDICATOR_INACTIVE_COLOR);
         width = renderComponentIndicator(xPos, top, weapBuf);
         xPos += width + IndicatorHorizPadding;
      }

      FontManager::popFontContext();
      return xPos - LoadoutIndicator::LoadoutIndicatorLeftPos - IndicatorHorizPadding;
   }

   // Standard BF mode: show weapons + modules.

   // If if we have no module, then this loadout has never been set, and there is nothing to render
   if(!loadout.isValid())
      return 0;

   S32 xPos = LoadoutIndicator::LoadoutIndicatorLeftPos;

   FontManager::pushFontContext(LoadoutIndicatorContext);

   // First, the weapons
   for(auto i = 0; i < ShipWeaponCount; i++)
   {
      r.setColor(loadout.isWeaponActive(i) ? *INDICATOR_ACTIVE_COLOR : *INDICATOR_INACTIVE_COLOR);

      S32 width = renderComponentIndicator(xPos, top, WeaponInfo::getWeaponInfo(loadout.getWeapon(i)).name.getString());

      xPos += width + IndicatorHorizPadding;
   }

   xPos += GapBetweenTheGroups;    // Small horizontal gap to separate the weapon indicators from the module indicators

   // Next, loadout modules
   for(auto i = 0; i < ShipModuleCount; i++)
   {
      ShipModule module = loadout.getModule(i);

      setModuleColor(r, module, loadout.isModulePrimaryActive(module), loadout.isModuleSecondaryActive(module));

      S32 width = renderComponentIndicator(xPos, top, ModuleInfo::getModuleInfo(module)->getName());

      xPos += width + IndicatorHorizPadding;
   }

   FontManager::popFontContext();

   return xPos - LoadoutIndicator::LoadoutIndicatorLeftPos - IndicatorHorizPadding;
}


// This should return the same width as doRender()
S32 LoadoutIndicator::getWidth() const
{
   // Xtank mode: body name + engine + treads + heat sinks + weapons
   if(!mXtankDesign.isXtankVehicle())
   {
      S32 bodyIdx = (S32)mXtankDesign.body;
      S32 width = getComponentIndicatorWidth(xtankBodyNames[bodyIdx]) + IndicatorHorizPadding;
      width += GapBetweenTheGroups;
      width += getComponentIndicatorWidth(xtankEngineInfos[(S32)mXtankDesign.engine].name) + IndicatorHorizPadding;
      width += getComponentIndicatorWidth(xtankTreadInfos[(S32)mXtankDesign.tread].name) + IndicatorHorizPadding;

      char hsBuf[16];
      dSprintf(hsBuf, sizeof(hsBuf), "HS: %d", (S32)mXtankDesign.heatSinks);
      width += getComponentIndicatorWidth(hsBuf) + IndicatorHorizPadding;

      width += GapBetweenTheGroups;
      for(S32 i = 0; i < WEAPON_SLOTS; i++)
      {
         XtankWeapon wt = mXtankDesign.weapons[i];
         char weapBuf[96];
         if(wt != XtankWeapon::NONE)
            dSprintf(weapBuf, sizeof(weapBuf), "#%d %s:%s", i + 1, getMountLabel(mXtankDesign.weaponMounts[i]), xtankWeaponInfos[(S32)wt].name);
         else
            dSprintf(weapBuf, sizeof(weapBuf), "#%d --", i + 1);
         width += getComponentIndicatorWidth(weapBuf) + IndicatorHorizPadding;
      }
      if(WEAPON_SLOTS > 0)
         width -= IndicatorHorizPadding;
      return width;
   }

   // Standard BF mode
   S32 width = 0;

   for(auto i = 0; i < ShipWeaponCount; i++)
      width += getComponentIndicatorWidth(WeaponInfo::getWeaponInfo(mCurrLoadout.getWeapon(i)).name.getString()) + IndicatorHorizPadding;

   width += GapBetweenTheGroups;

   for(auto i = 0; i < ShipModuleCount; i++)
      width += getComponentIndicatorWidth(ModuleInfo::getModuleInfo(mCurrLoadout.getModule(i))->getName()) + IndicatorHorizPadding;

   width -= IndicatorHorizPadding;

   return width;
}


// Draw weapon indicators at top of the screen, runs on client
S32 LoadoutIndicator::render(ClientGame *game) const
{
   S32 top;

   DisplayMode windowMode = game->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode");

   // Old loadout
   top = Parent::prepareToRenderFromDisplay(windowMode, LoadoutIndicatorTopPos - 1, LoadoutIndicatorHeight + 1);
   if(top != NO_RENDER)
   {
      VehicleDesign empty;  // For the previous-loadout slot we don't track a prev xtank design
      doRender(mPrevLoadout, empty, game, top);
      doneRendering();
   }

   // Current loadout
   top = Parent::prepareToRenderToDisplay(windowMode, LoadoutIndicatorTopPos, LoadoutIndicatorHeight);
   S32 width = doRender(mCurrLoadout, mXtankDesign, game, top);
   doneRendering();

   return width;
}


} } // Nested namespace
