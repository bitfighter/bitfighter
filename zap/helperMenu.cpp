//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "helperMenu.h"

#include "UIGame.h"              // For mGameUserInterface
#include "UIInstructions.h"

#include "UIManager.h"
#include "ClientGame.h"
#include "FontManager.h"

#include "Joystick.h"
#include "JoystickRender.h"

#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"


namespace Zap
{

using namespace UI;

// Constuctor
HelperMenu::HelperMenu()
{
   mClientGame = NULL;
   mHelperManager = NULL;

   mOldBottom = 0;
   mOldCount = 0;
}


// Destructor
HelperMenu::~HelperMenu()
{
   // Do nothing
}


void HelperMenu::initialize(ClientGame *game, HelperManager *manager)
{
   mHelperManager = manager;
   mClientGame = game;
}


// Static method, for testing
InputCode HelperMenu::getInputCodeForOption(const OverlayMenuItem *items, S32 itemCount, U32 index, bool keyBut)
{
   for(S32 i = 0; i < itemCount; i++)
      if(items[i].itemIndex == index)
         return keyBut ? items[i].key : items[i].button;

   return KEY_UNKNOWN;
}


void HelperMenu::onActivated()    
{
   mHorizLabelOffset = 0;

   // Activate parent classes
   Slider::onActivated();
   Scroller::onActivated();
}


const char *HelperMenu::getCancelMessage() const
{
   return "";
}


InputCode HelperMenu::getActivationKey()
{
   return KEY_NONE;
}


// Exit helper mode by entering play mode
void HelperMenu::exitHelper() 
{ 
   onDeactivated();
   mClientGame->getUIManager()->getUI<GameUserInterface>()->exitHelper();
}


static S32 LeftMargin = 8;          // Left margin where controller symbols/keys are rendered
static S32 ButtonLabelGap = 9;      // Space between button/key rendering and menu item


// Returns total width of the helper
 S32 HelperMenu::getTotalDisplayWidth(S32 widthOfButtons, S32 widthOfTextBlock) const
{
   return widthOfButtons + widthOfTextBlock + LeftMargin + ButtonLabelGap + MENU_PADDING;
}


// Returns visible width of the helper
 S32 HelperMenu::getCurrentDisplayWidth(S32 widthOfButtons, S32 widthOfTextBlock) const
{
   return getWidth() + (S32)getInsideEdge();
}


 // Count how many items we will be displaying -- some may be hidden
 // Should be precalcuated when items change!!
static S32 getDisplayItemCount(const OverlayMenuItem *items, S32 itemCount)
{
   S32 displayItems = 0;
   for(S32 i = 0; i < itemCount; i++)
      if(items[i].showOnMenu)
         displayItems++;

   return displayItems;
}


extern void drawHorizLine(S32 x1, S32 x2, S32 y);

// Set a bunch of display geometry parameters -- there are more in the .h file
static const S32 MENU_LEGEND_FONT_SIZE = 11;    // Smaller font of lengend items on QuickChat menus
static const S32 TITLE_FONT_SIZE       = 20;    // Size of title of menu
static const S32 GrayLineBuffer        = 10;

static const S32 TitleHeight = TITLE_FONT_SIZE + GrayLineBuffer;
static const S32 InstructionHeight = MENU_LEGEND_FONT_SIZE;


S32 HelperMenu::getLegendHeight() const
{
   if(mLegend)
      return MENU_LEGEND_FONT_SIZE + MENU_FONT_SPACING;

   return 0;
}

// Total height of the menu
S32 HelperMenu::getMenuHeight() const
{
   S32 displayItems = getDisplayItemCount(mCurrentRenderItems, mCurrentRenderCount);

   // Height of variable menu parts
   const S32 itemsHeight  = displayItems * (MENU_FONT_SIZE + MENU_FONT_SPACING) + MENU_PADDING + GrayLineBuffer;

   return MENU_PADDING + TitleHeight + itemsHeight + getLegendHeight() + InstructionHeight + BottomPadding;     
}


S32 HelperMenu::getMenuBottomPos() const
{
   return MENU_TOP + getMenuHeight();
}


// Oh, this is so ugly and convoluted!  Drawing things on the screen is so messy!
void HelperMenu::drawItemMenu(S32 widthOfButtons, S32 widthOfTextBlock) const
{
   if(mCurrentRenderCount == 0)
      return;

   glPushMatrix();
   glTranslate(getInsideEdge(), 0);

   static const Color baseColor(Colors::red);

   // Total height of the menu
   const S32 totalHeight = getMenuHeight();

   S32 newBottom = getMenuBottomPos();

   static const S32 yStartPos = MENU_TOP + MENU_PADDING;   

   S32 yPos = yStartPos;

   // If we are transitioning between items of different sizes, we will gradually change the rendered size during the transition
   // Generally, the top of the menu will stay in place, while the bottom will be adjusted.  Therefore, lower items need
   // to be offset by the transitionOffset which we will calculate below.  MenuBottom will be the actual bottom of the menu
   // adjusted for the transition effect.
   S32 menuBottom = getTransitionPos(mOldBottom, newBottom);

   FontManager::pushFontContext(HelperMenuContext);

   S32 grayLineLeft   = 20;
   S32 grayLineRight  = getWidth() - 20;
   S32 grayLineCenter = (grayLineLeft + grayLineRight) / 2;

   static const Color frameColor = Colors::red35;
   renderSlideoutWidgetFrame(0, MENU_TOP, getWidth(), menuBottom - MENU_TOP, frameColor);

   // Draw the title (above gray line)
   glColor(baseColor);
   
   FontManager::pushFontContext(HelperMenuHeadlineContext);
   drawCenteredString(grayLineCenter, yPos, TITLE_FONT_SIZE, mTitle);
   FontManager::popFontContext();

   yPos += TitleHeight;

   // Gray line
   glColor(Colors::gray20);
   drawHorizLine(grayLineLeft, grayLineRight, yPos + 2);

   yPos += GrayLineBuffer;

   // Draw menu items (below gray line)
   drawMenuItems(mPrevRenderItems,    mPrevRenderCount,    yPos + 2, menuBottom, false, mHorizLabelOffset);
   drawMenuItems(mCurrentRenderItems, mCurrentRenderCount, yPos,     menuBottom, true,  0);      

   yPos += getMenuHeight(); 

   // Adjust for any transition that might be going on that is changing the overall menu height.  menuBottom is the rendering location
   // of the bottom fo the menu, newBottom is the target bottom location after the transition has ocurred.
   yPos += menuBottom - newBottom;

   S32 legendHeight = getLegendHeight();

   if(mLegend)
      renderLegend(grayLineCenter, yPos - legendHeight - 3, *mLegend);

   yPos += legendHeight;

   renderPressEscapeToCancel(grayLineCenter, yPos, baseColor, getGame()->getInputMode());

   FontManager::popFontContext();

   glPopMatrix();
}


S32 HelperMenu::getButtonWidth(const OverlayMenuItem *items, S32 itemCount) const
{
   // Determine whether to show keys or joystick buttons on menu
   InputMode inputMode = getGame()->getInputMode();

   S32 buttonWidth = 0;
   for(S32 i = 0; i < itemCount; i++)
   {
      if(!items[i].showOnMenu)
         continue;

      InputCode code = (inputMode == InputModeJoystick) ? items[i].button : items[i].key;
      S32 w = JoystickRender::getControllerButtonRenderedSize(code);

      buttonWidth = MAX(buttonWidth, w);
   }

   return buttonWidth;
}


void HelperMenu::setExpectedWidth_MidTransition(S32 width)
{
   mHorizLabelOffset =  width - getWidth();
   Slider::setExpectedWidth_MidTransition(width);
}


// Render a set of menu items.  Break this code out to make transitions easier (when we'll be rendering two sets of items).
void HelperMenu::drawMenuItems(const OverlayMenuItem *items, S32 count, S32 top, S32 bottom, bool newItems, S32 horizOffset) const
{
   if(!items)
      return;

   S32 height = 0;

   // Calculate height of all our items, ignorning those that are hidden
   for(S32 i = 0; i < count; i++)
      if(items[i].showOnMenu)
         height += MENU_FONT_SIZE + MENU_FONT_SPACING;

   S32 oldHeight = (MENU_FONT_SIZE + MENU_FONT_SPACING) * mOldCount;

   //// Determine whether to show keys or joystick buttons on menu
   InputMode inputMode = getGame()->getInputMode();

//// For testing purposes -- toggles between keyboard and joystick renders --> Comment this out when not testing
//   if(Platform::getRealMilliseconds() % 2000 > 1000)
//      inputMode = InputModeJoystick;
//#  ifndef TNL_DEBUG
//#     error "This block must be removed in any production builds"
//#  endif
/////

   S32 buttonWidth = getButtonWidth(items, count);
   DisplayMode displayMode = getGame()->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>(IniKey::WindowMode);
   
   S32 yPos;

   if(newItems)      // Draw the new items we're transitioning to
      yPos = prepareToRenderToDisplay(displayMode, top, oldHeight, height);
   else              // Draw the old items we're transitioning away from
      yPos = prepareToRenderFromDisplay(displayMode, top, oldHeight, height);

   yPos += 2;        // Aesthetics

   for(S32 i = 0; i < count; i++)
   {
      // Skip hidden options!
      if(!items[i].showOnMenu)
         continue;

      InputCode code = (inputMode == InputModeJoystick) ? items[i].button : items[i].key;    // Get the input code for the thing we want to render

      // Render key in white, or, if there is a legend, in the color of the adjacent item
      const Color *buttonOverrideColor = items[i].buttonOverrideColor;
      
      const Color *itemColor = items[i].itemColor;

      // Need to add buttonWidth / 2 because renderControllerButton() centers on passed coords
      JoystickRender::renderControllerButton(LeftMargin + horizOffset + (F32)buttonWidth / 2, 
                                             (F32)yPos - 1, Joystick::SelectedPresetIndex, code, 
                                             buttonOverrideColor); 
      glColor(itemColor);  

      S32 xPos = LeftMargin + buttonWidth + ButtonLabelGap + horizOffset;
      S32 textWidth = drawStringAndGetWidth(xPos, yPos, MENU_FONT_SIZE, items[i].name); 

      // Render help string, if one is available
      if(strcmp(items[i].help, "") != 0)
      {
         glColor(items[i].helpColor);    
         xPos += textWidth + ButtonLabelGap;
         drawString(xPos, yPos, MENU_FONT_SIZE, items[i].help);
      }

      yPos += MENU_FONT_SIZE + MENU_FONT_SPACING;
   }

   doneRendering();
}


void HelperMenu::renderPressEscapeToCancel(S32 xPos, S32 yPos, const Color &baseColor, InputMode inputMode) const
{
   glColor(baseColor);

   // RenderedSize will be -1 if the button is not defined
   if(inputMode == InputModeKeyboard)
      drawStringfc((F32)xPos, (F32)yPos, (F32)MENU_LEGEND_FONT_SIZE, 
                  "Press [%s] to cancel", InputCodeManager::inputCodeToString(KEY_ESCAPE));
   else
   {

      static const SymbolString JoystickInstructions(
            "Press [[Back]] to cancel", 
            mClientGame->getSettings()->getInputCodeManager(), MenuHeaderContext, 
            MENU_LEGEND_FONT_SIZE, false, AlignmentCenter);

      JoystickInstructions.render(Point(xPos + 4, yPos));
   }
}


void HelperMenu::renderLegend(S32 x, S32 y, const Vector<HelperMenuLegendItem> &legend) const
{
   const S32 SPACE_BETWEEN_LEGEND_ITEMS = 7;

   S32 width = 0;
   y += MENU_FONT_SPACING;

   // First, get the total width so we can center poperly
   for(S32 i = 0; i < legend.size(); i++)
      width += getStringWidth(MENU_LEGEND_FONT_SIZE, legend[i].text.c_str()) + SPACE_BETWEEN_LEGEND_ITEMS;

   width -= SPACE_BETWEEN_LEGEND_ITEMS;

   x -= width / 2;

   for(S32 i = 0; i < legend.size(); i++)
   {
      glColor(legend[i].color);
      x += drawStringAndGetWidth(x, y, MENU_LEGEND_FONT_SIZE, legend[i].text.c_str()) + SPACE_BETWEEN_LEGEND_ITEMS;
   }
}


// Calculate the width of the widest item in items
S32 HelperMenu::getMaxItemWidth(const OverlayMenuItem *items, S32 count) const
{
   S32 maxWidth = 0;

   for(S32 i = 0; i < count; i++)
   {
      S32 width = getStringWidth(HelperMenuContext, MENU_FONT_SIZE, items[i].name) + getStringWidth(MENU_FONT_SIZE, items[i].help);
      if(width > maxWidth)
         maxWidth = width;
   }

   return maxWidth;
}


ClientGame *HelperMenu::getGame() const
{
   return mClientGame;
}


// Returns true if key was handled, false if it should be further processed
bool HelperMenu::processInputCode(InputCode inputCode)
{
   // First, check navigation keys.  When in keyboard mode, we allow the loadout key to toggle menu on and off...
   // we can't do this in joystick mode because it is likely that the loadout key is also used to select items
   // from the loadout menu.
   if(inputCode == KEY_ESCAPE  || inputCode == BUTTON_DPAD_LEFT || inputCode == BUTTON_BACK || 
      (getActivationKeyClosesHelper() && getGame()->getInputMode() == InputModeKeyboard && inputCode == getActivationKey()) )
   {
      exitHelper();      

      mClientGame->displayMessage(Colors::ErrorMessageTextColor, getCancelMessage());

      return true;
   }

   return false;
}


bool HelperMenu::getActivationKeyClosesHelper()
{
   return true;
}


void HelperMenu::onTextInput(char ascii)
{
   // Do nothing (overridden by ChatHelper)
}


void HelperMenu::activateHelp(UIManager *uiManager)
{
    uiManager->activate<InstructionsUserInterface>();
}


bool HelperMenu::isMovementDisabled() const { return false; }
bool HelperMenu::isChatDisabled() const     { return true;  }


void HelperMenu::idle(U32 deltaT) 
{
   // Idle the parent classes
   Slider::idle(deltaT);
   Scroller::idle(deltaT);

   // Once scroll effect is over, need to save some values for next time
   // Not sure this is the right place... we'll see!
   if(!Scroller::isActive())
   {
      mOldBottom = getTransitionPos(mOldBottom, getMenuBottomPos());
      mOldCount =  getDisplayItemCount(mCurrentRenderItems, mCurrentRenderCount);
   }
}


// Gets run when closing animation is complet
void HelperMenu::onWidgetClosed()
{
   Slider::onWidgetClosed();
   mHelperManager->doneClosingHelper();
}


};
