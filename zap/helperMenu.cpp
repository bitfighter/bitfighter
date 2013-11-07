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
   // Do nothing
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
   Slider::onDeactivated();
   mClientGame->getUIManager()->getUI<GameUserInterface>()->exitHelper();
}


static S32 LeftMargin = 8;          // Left margin where controller symbols/keys are rendered
static S32 ButtonLabelGap = 9;      // Space between button/key rendering and menu item


extern void drawVertLine  (S32 x,  S32 y1, S32 y2);
extern void drawHorizLine (S32 x1, S32 x2, S32 y );

void HelperMenu::drawItemMenu(const char *title, const OverlayMenuItem *items, S32 count, const OverlayMenuItem *prevItems, S32 prevCount,
                              const char **legendText, const Color **legendColors, S32 legendCount)
{
   glPushMatrix();
   glTranslate(getInsideEdge(), 0, 0);

   static const Color baseColor(Colors::red);

   TNLAssert(glIsEnabled(GL_BLEND), "Expect blending to be on");

   S32 displayItems = 0;

   // Count how many items we will be displaying -- some may be hidden
   for(S32 i = 0; i < count; i++)
      if(items[i].showOnMenu)
         displayItems++;

   bool hasLegend = legendCount > 0;

   const S32 grayLineBuffer = 10;

   // Height of menu parts
   const S32 topPadding        = MENU_PADDING;
   const S32 titleHeight       = TITLE_FONT_SIZE + grayLineBuffer ;
   const S32 itemsHeight       = displayItems * (MENU_FONT_SIZE + MENU_FONT_SPACING) + MENU_PADDING + grayLineBuffer;
   const S32 legendHeight      = (hasLegend ? MENU_LEGEND_FONT_SIZE + MENU_FONT_SPACING : 0); 
   const S32 instructionHeight = MENU_LEGEND_FONT_SIZE;
   const S32 bottomPadding     = MENU_PADDING;

   // Total height of the menu
   const S32 totalHeight = topPadding + titleHeight + itemsHeight + legendHeight + instructionHeight + bottomPadding;     

   S32 yPos = MENU_TOP + topPadding;
   S32 newBottom = MENU_TOP + totalHeight;

   // If we are transitioning between items of different sizes, we will gradually change the rendered size during the transition
   // Generally, the top of the menu will stay in place, while the bottom will be adjusted.  Therefore, lower items need
   // to be offset by the transitionOffset which we will calculate below.  MenuBottom will be the actual bottom of the menu
   // adjusted for the transition effect.
   S32 menuBottom = getTransitionPos(mOldBottom, newBottom);

   // Once scroll effect is over, need to save some values for next time
   if(!Scroller::isActive())
   {
      mOldBottom = menuBottom;
      mOldCount = displayItems;
   }

   FontManager::pushFontContext(HelperMenuContext);

   // Get the left edge of where the text portion of the menu items should be rendered
   S32 itemIndent = calcLeftMarginForTextPortionOfEntry(items, count) + LeftMargin + ButtonLabelGap;

   S32 interiorEdge = mTextPortionOfItemWidth + itemIndent + MENU_PADDING;

   S32 grayLineLeft   = 20;
   S32 grayLineRight  = interiorEdge - 20;
   S32 grayLineCenter = (grayLineLeft + grayLineRight) / 2;

   static const Color frameColor(.35,0,0);
   renderSlideoutWidgetFrame(0, MENU_TOP, interiorEdge, menuBottom - MENU_TOP, frameColor);

   // Draw the title (above gray line)
   glColor(baseColor);
   
   FontManager::pushFontContext(HelperMenuHeadlineContext);
   drawCenteredString(grayLineCenter, yPos, TITLE_FONT_SIZE, title);
   FontManager::popFontContext();

   yPos += titleHeight;

   // Gray line
   glColor(Colors::gray20);

   drawHorizLine(grayLineLeft, grayLineRight, yPos + 2);

   yPos += grayLineBuffer;


   // Draw menu items (below gray line)
   drawMenuItems(prevItems, prevCount, yPos + 2, menuBottom, false, false);
   drawMenuItems(items,     count,     yPos,     menuBottom, true,  false);      

   // itemsHeight includes grayLineBuffer, transitionOffset accounts for potentially changing menu height during transition
   yPos += itemsHeight; 

   // Adjust for any transition that might be going on that is changing the overall menu height.  menuBottom is the rendering location
   // of the bottom fo the menu, newBottom is the target bottom location after the transition has ocurred.
   yPos += menuBottom - newBottom;

   if(hasLegend)
      renderLegend(grayLineCenter, yPos - legendHeight - 3, legendText, legendColors, legendCount);

   yPos += legendHeight;

   renderPressEscapeToCancel(grayLineCenter, yPos, baseColor, getGame()->getInputMode());

   FontManager::popFontContext();

   glPopMatrix();
}


S32 HelperMenu::calcLeftMarginForTextPortionOfEntry(const OverlayMenuItem *items, S32 itemCount)
{
   S32 displayItems = 0;

   // Determine whether to show keys or joystick buttons on menu
   InputMode inputMode = getGame()->getInputMode();

   S32 buttonWidth = 0;
   for(S32 i = 0; i < itemCount; i++)
   {
      if(!items[i].showOnMenu)
         continue;

      InputCode code = (inputMode == InputModeJoystick) ? items[i].button : items[i].key;
      S32 w = JoystickRender::getControllerButtonRenderedSize(Joystick::SelectedPresetIndex, code);

      buttonWidth = MAX(buttonWidth, w);
   }

   return buttonWidth;
}


// Render a set of menu items.  Break this code out to make transitions easier (when we'll be rendering two sets of items).
void HelperMenu::drawMenuItems(const OverlayMenuItem *items, S32 count, S32 top, S32 bottom, bool newItems, bool renderKeysWithItemColor)
{
   if(!items)
      return;

   S32 displayItems = 0;

   // Count how many items we will be displaying -- some may be hidden
   for(S32 i = 0; i < count; i++)
      if(items[i].showOnMenu)
         displayItems++;

   S32 height    = (MENU_FONT_SIZE + MENU_FONT_SPACING) * displayItems;
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

   S32 buttonWidth = calcLeftMarginForTextPortionOfEntry(items, count);

   S32 yPos;

   if(newItems)      // Draw the new items we're transitioning to
      yPos = prepareToRenderToDisplay(getGame()->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode"), 
                                      top, oldHeight, height);
   else              // Draw the old items we're transitioning away from
      yPos = prepareToRenderFromDisplay(getGame()->getSettings()->getIniSettings()->mSettings.getVal<DisplayMode>("WindowMode"), 
                                      top, oldHeight, height);

   yPos += 2;        // Aesthetics

   for(S32 i = 0; i < count; i++)
   {
      // Skip hidden options!
      if(!items[i].showOnMenu)
         continue;

      InputCode code = (inputMode == InputModeJoystick) ? items[i].button : items[i].key;    // Get the input code for the thing we want to render

      // Render key in white, or, if there is a legend, in the color of the adjacent item
      const Color *buttonOverrideColor = items[i].buttonOverrideColor;  // renderKeysWithItemColor ? items[i].itemColor : NULL; 
      
      const Color *itemColor = items[i].itemColor;

      // Need to add buttonWidth / 2 because renderControllerButton() centers on passed coords
      JoystickRender::renderControllerButton(LeftMargin + (F32)buttonWidth / 2, (F32)yPos - 1, Joystick::SelectedPresetIndex, code, buttonOverrideColor); 

      glColor(itemColor);  

      S32 textWidth = drawStringAndGetWidth(LeftMargin + buttonWidth + ButtonLabelGap, yPos, MENU_FONT_SIZE, items[i].name); 

      // Render help string, if one is available
      if(strcmp(items[i].help, "") != 0)
      {
         glColor(items[i].helpColor);    
         drawString(LeftMargin + buttonWidth + ButtonLabelGap + textWidth + ButtonLabelGap, yPos, MENU_FONT_SIZE, items[i].help);
      }

      yPos += MENU_FONT_SIZE + MENU_FONT_SPACING;
   }

   doneRendering();
}


void HelperMenu::renderPressEscapeToCancel(S32 xPos, S32 yPos, const Color &baseColor, InputMode inputMode)
{
   glColor(baseColor);

   // RenderedSize will be -1 if the button is not defined
   if(inputMode == InputModeKeyboard)
      drawStringfc((F32)xPos, (F32)yPos, (F32)MENU_LEGEND_FONT_SIZE, 
                  "Press [%s] to cancel", InputCodeManager::inputCodeToString(KEY_ESCAPE));
   else
   {
      S32 butSize = JoystickRender::getControllerButtonRenderedSize(Joystick::SelectedPresetIndex, BUTTON_BACK);

      xPos += drawStringAndGetWidth(xPos, yPos, MENU_LEGEND_FONT_SIZE, "Press ") + 4;
      JoystickRender::renderControllerButton(F32(xPos + 4), F32(yPos), Joystick::SelectedPresetIndex, BUTTON_BACK);
      xPos += butSize;
      glColor(baseColor);
      drawString(xPos, yPos, MENU_LEGEND_FONT_SIZE, "to cancel");
   }
}


void HelperMenu::renderLegend(S32 x, S32 y, const char **legendText, const Color **legendColors, S32 legendCount)
{
   S32 width = 0;
   y += MENU_FONT_SPACING;

   const S32 SPACE_BETWEEN_LEGEND_ITEMS = 7;

   // First, get the total width so we can center poperly
   for(S32 i = 0; i < legendCount; i++)
      width += getStringWidth(MENU_LEGEND_FONT_SIZE, legendText[i]) + SPACE_BETWEEN_LEGEND_ITEMS;

   x -= width / 2;

   for(S32 i = 0; i < legendCount; i++)
   {
      glColor(legendColors[i]);
      x += drawStringAndGetWidth(x, y, MENU_LEGEND_FONT_SIZE, legendText[i]) + SPACE_BETWEEN_LEGEND_ITEMS;
   }
}


// Calculate the width of the widest item in items
S32 HelperMenu::getMaxItemWidth(const OverlayMenuItem *items, S32 count)
{
   S32 width = -1;
   for(S32 i = 0; i < count; i++)
   {
      S32 w = getStringWidth(HelperMenuContext, MENU_FONT_SIZE, items[i].name) + getStringWidth(MENU_FONT_SIZE, items[i].help);
      if(w > width)
         width = w;
   }

   return width;
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
   if(inputCode == KEY_ESCAPE  || inputCode == BUTTON_DPAD_LEFT ||
      inputCode == BUTTON_BACK || 
      (getGame()->getInputMode() == InputModeKeyboard && inputCode == getActivationKey()) )
   {
      exitHelper();      

      if(mClientGame->getSettings()->getIniSettings()->mSettings.getVal<YesNo>("VerboseHelpMessages"))
         mClientGame->displayMessage(Colors::paleRed, getCancelMessage());

      return true;
   }

   return false;
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
}


// Gets run when closing animation is complet
void HelperMenu::onWidgetClosed()
{
   mHelperManager->doneClosingHelper();
}


};
