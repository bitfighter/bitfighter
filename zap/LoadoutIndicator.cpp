//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LoadoutIndicator.h"

#include "ClientGame.h"
#include "FontManager.h"

#include "Colors.h"

#include "RenderUtils.h"
#include "stringUtils.h"


using namespace Zap;

namespace Zap { namespace UI {


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
   mCurrLoadout.resetLoadout();
   mPrevLoadout.resetLoadout();
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
static S32 renderComponentIndicator(S32 xPos, S32 yPos, const Color &color, const char *name)
{
   // Adjustment factors that just make the indicators look better.  Trust the math, but verify the aesthetics.
   const S32 xAdjustment = 2;
   const S32 yAdjustment = -1;

   // Draw the weapon or module name (n.b.: If you change the lcase, do the same in getComponentIndicatorWidth)
   S32 textWidth = RenderUtils::drawStringAndGetWidth_fixed(xPos + IndicatorHorizPadding + xAdjustment,
                                                            yPos + IndicatorVertPadding + IndicatorFontSize + yAdjustment,
                                                            IndicatorFontSize, 
                                                            color, 
                                                            lcase(name).c_str());

   S32 rectWidth = getComponentRectWidth(textWidth);

   RenderUtils::drawFancyBox(xPos, yPos, xPos + rectWidth, yPos + IndicatorHeight, IndicatorVertPadding, 2, color);

   return rectWidth;
}


static S32 getComponentIndicatorWidth(const char *name)
{
   return getComponentRectWidth(RenderUtils::getStringWidth(IndicatorFontSize, lcase(name).c_str()));
}


static const S32 GapBetweenTheGroups = 20;

static const Color &INDICATOR_INACTIVE_COLOR = Colors::green80;
static const Color &INDICATOR_ACTIVE_COLOR   = Colors::red80;
static const Color &INDICATOR_PASSIVE_COLOR  = Colors::yellow;


static const Color &getRenderColor(ShipModule module, bool isModuleActive, bool isSecondaryActive)
{
   // Always change to orange if module secondary is fired
   if(gModuleInfo[module].hasSecondary() && isSecondaryActive)
      return Colors::orange67;

   if(gModuleInfo[module].getPrimaryUseType() == ModulePrimaryUsePassive ||   // Armor
      gModuleInfo[module].getPrimaryUseType() == ModulePrimaryUseHybrid)      // Sensor
   {
      return INDICATOR_PASSIVE_COLOR;
   }
   
   if(isModuleActive)
      return INDICATOR_ACTIVE_COLOR;
   
   return INDICATOR_INACTIVE_COLOR;


}

// Returns width
static S32 doRender(const LoadoutTracker &loadout, const ClientGame *game, S32 top)
{
   // If if we have no module, then this loadout has never been set, and there is nothing to render
   if(!loadout.isValid())  
      return 0;

   S32 xPos = LoadoutIndicator::LoadoutIndicatorLeftPos;

   FontManager::pushFontContext(LoadoutIndicatorContext);
   
   // First, the weapons
   for(S32 i = 0; i < ShipWeaponCount; i++)
   {
      const Color &color = loadout.isWeaponActive(i) ? INDICATOR_ACTIVE_COLOR : INDICATOR_INACTIVE_COLOR;

      S32 width = renderComponentIndicator(xPos, top, color, WeaponInfo::getWeaponInfo(loadout.getWeapon(i)).name.getString());

      xPos += width + IndicatorHorizPadding;
   }

   xPos += GapBetweenTheGroups;    // Small horizontal gap to separate the weapon indicators from the module indicators

   // Next, loadout modules
   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      ShipModule module = loadout.getModule(i);
      const Color &color = getRenderColor(module, loadout.isModulePrimaryActive(module), loadout.isModuleSecondaryActive(module));

      S32 width = renderComponentIndicator(xPos, top, color, ModuleInfo::getModuleInfo(module)->getName());

      xPos += width + IndicatorHorizPadding;
   }

   FontManager::popFontContext();

   return xPos - LoadoutIndicator::LoadoutIndicatorLeftPos - IndicatorHorizPadding;
}


// This should return the same width as doRender()
S32 LoadoutIndicator::getWidth() const
{
   S32 width = 0;

   for(U32 i = 0; i < (U32)ShipWeaponCount; i++)
      width += getComponentIndicatorWidth(WeaponInfo::getWeaponInfo(mCurrLoadout.getWeapon(i)).name.getString()) + IndicatorHorizPadding;

   width += GapBetweenTheGroups;

   for(U32 i = 0; i < (U32)ShipModuleCount; i++)
      width += getComponentIndicatorWidth(ModuleInfo::getModuleInfo(mCurrLoadout.getModule(i))->getName()) + IndicatorHorizPadding;

   width -= IndicatorHorizPadding;

   return width;
}


// Draw weapon indicators at top of the screen, runs on client
S32 LoadoutIndicator::render(const ClientGame *game) const
{
   S32 top;

   DisplayMode windowMode = game->getSettings()->getSetting<DisplayMode>(IniKey::WindowMode);

   // Old loadout
   top = prepareToRenderFromDisplay(windowMode, LoadoutIndicatorTopPos - 1, LoadoutIndicatorHeight + 1);
   if(top != NO_RENDER)
   {
      doRender(mPrevLoadout, game, top);
      doneRendering();
   }

   // Current loadout
   top = prepareToRenderToDisplay(windowMode, LoadoutIndicatorTopPos, LoadoutIndicatorHeight);
   S32 width = doRender(mCurrLoadout, game, top);
   doneRendering();

   return width;
}


} } // Nested namespace
