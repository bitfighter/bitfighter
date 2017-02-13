//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "JoystickRender.h"
#include "Joystick.h"
#include "InputCode.h"
#include "SymbolShape.h"
#include "Colors.h"

#include "RenderUtils.h"

namespace Zap
{

using namespace UI;


JoystickRender::JoystickRender()
{
   // Do nothing
}


JoystickRender::~JoystickRender()
{
   // Do nothing
}


inline const Color &JoystickRender::getButtonColor(bool activated)
{
   if(activated)
      return Colors::red;

   return Colors::white;
}


// Render dpad graphic
void JoystickRender::renderDPad(const Point &center, bool upActivated, bool downActivated, bool leftActivated,
                                bool rightActivated, const char *msg1, const char *msg2)
{
   drawDPadUp   (center + Point(0, -16), getButtonColor(upActivated));
   drawDPadDown (center + Point(0,  16), getButtonColor(downActivated));
   drawDPadLeft (center + Point(-16, 0), getButtonColor(leftActivated));
   drawDPadRight(center + Point(16,  0), getButtonColor(rightActivated));

   // Label the graphic
   if(strcmp(msg1, "") == 0)    // That is, != "".  Remember, kids, strcmp returns 0 when strings are identical!
   {
      S32 size = 12;
      RenderUtils::drawStringc(center.x, center.y + size + 27, size, Colors::white, msg1);
   }

   if(strcmp(msg2, "") == 0)
   {
      S32 size = 10;
      RenderUtils::drawStringc(center.x, center.y + size + 42, size, Colors::white, msg2);
   }
}


static S32 SymbolPadding = 6;       // Just some padding we throw around our symbols to make them look hot

S32 JoystickRender::getControllerButtonRenderedSize(InputCode inputCode)
{
   // Return keyboard key size, just in case
   if(!InputCodeManager::isControllerButton(inputCode))
      return SymbolKey::getRenderWidth(InputCodeManager::inputCodeToString(inputCode));

   // Get joystick button size
   S16 button = InputCodeManager::inputCodeToControllerButton(inputCode);

   Joystick::ButtonInfo buttonInfo = Joystick::getButtonInfo(button);

   switch(buttonInfo.buttonShape)
   {
      case Joystick::ButtonShapeRound:             return buttonHalfHeight * 2          + SymbolPadding;
      case Joystick::ButtonShapeRect:              return rectButtonWidth               + SymbolPadding;
      case Joystick::ButtonShapeSmallRect:         return smallRectButtonWidth          + SymbolPadding;
      case Joystick::ButtonShapeRoundedRect:       return rectButtonWidth               + SymbolPadding;
      case Joystick::ButtonShapeSmallRoundedRect:  return smallRectButtonWidth          + SymbolPadding;
      case Joystick::ButtonShapeHorizEllipse:      return horizEllipseButtonRadiusX * 2 + SymbolPadding;
      case Joystick::ButtonShapeRightTriangle:     return rectButtonWidth               + SymbolPadding;
      default:                                     return rectButtonWidth               + SymbolPadding;
   }
}

// Thinking...
//class SymbolShape 
//{
//   void render(S32 x, S32 y);
//};
//
//
//class Symbol 
//{
//   SymbolShape shape;
//   string label;
//   Color color;
//
//   void render(S32 x, S32 y);
//};


// Renders something resembling a controller button or keyboard key
// Note: buttons are with the given x coordinate as their _center_, y coordinate given is the top of the button
bool JoystickRender::renderControllerButton(F32 centerX, F32 bottomY, InputCode inputCode, const Color *overrideRenderColor)
{
   // Set the basic color, could be overridden later
   const Color &color = overrideRenderColor ? *overrideRenderColor : Colors::white;

   // Render keyboard keys, just in case
   if(!InputCodeManager::isControllerButton(inputCode))
   {
      // The y coord we pass here will be the bottom of the key
      SymbolKey(InputCodeManager::inputCodeToString(inputCode), color).render(centerX, bottomY, AlignmentCenter);
      return true;
   }

   S16 button = InputCodeManager::inputCodeToControllerButton(inputCode);

   // Don't render if button doesn't exist
   if(button == SDL_CONTROLLER_BUTTON_INVALID)
      return false;

   Joystick::ButtonInfo buttonInfo = Joystick::getButtonInfo(button);
   Joystick::ButtonShape buttonShape = buttonInfo.buttonShape;

   const char *label = buttonInfo.label.c_str();
   const Color *buttonColor = &buttonInfo.color;

   // Note: the x coordinate is already at the center
   Point location(centerX, bottomY);
   Point center = location + Point(0, buttonHalfHeight);

   /////
   // 1. Render joystick button shape
   switch(buttonShape)
   {
      case Joystick::ButtonShapeRect:
         RenderUtils::drawRoundedRect(center, rectButtonWidth, rectButtonHeight, 3, color);
         break;
      case Joystick::ButtonShapeSmallRect:
         RenderUtils::drawRoundedRect(center, smallRectButtonWidth, smallRectButtonHeight, 3, color);
         break;
      case Joystick::ButtonShapeRoundedRect:
         RenderUtils::drawRoundedRect(center, rectButtonWidth, rectButtonHeight, 5, color);
         break;
      case Joystick::ButtonShapeSmallRoundedRect:
         RenderUtils::drawRoundedRect(center, smallRectButtonWidth, smallRectButtonHeight, 5, color);
         break;
      case Joystick::ButtonShapeHorizEllipse:
         RenderUtils::drawEllipse(center, horizEllipseButtonRadiusX, horizEllipseButtonRadiusY, color);
         break;
      case Joystick::ButtonShapeRightTriangle:
         location = location + Point(-rightTriangleWidth / 4.0f, 0);  // Need to off-center the label slightly for this button
         drawButtonRightTriangle(center, color);
         break;

      // DPad
      case Joystick::ButtonShapeDPadUp:
         drawDPadUp(center, color);
         break;
      case Joystick::ButtonShapeDPadDown:
         drawDPadDown(center, color);
         break;
      case Joystick::ButtonShapeDPadLeft:
         drawDPadLeft(center, color);
         break;
      case Joystick::ButtonShapeDPadRight:
         drawDPadRight(center, color);
         break;

      case Joystick::ButtonShapeRound:
      default:
         RenderUtils::drawCircle(center, (F32)buttonHalfHeight, color);
         break;
   }

   /////
   // 2. Render joystick label or symbol

   // Change color of label to the preset (default white)
   const Color &labelColor = overrideRenderColor ? *overrideRenderColor : *buttonColor;
      
   switch(buttonInfo.buttonSymbol)
   {
      case Joystick::ButtonSymbolPsCircle:
         drawPlaystationCircle(center);
         break;
      case Joystick::ButtonSymbolPsCross:
         drawPlaystationCross(center);
         break;
      case Joystick::ButtonSymbolPsSquare:
         drawPlaystationSquare(center);
         break;
      case Joystick::ButtonSymbolPsTriangle:
         drawPlaystationTriangle(center, labelColor);
         break;
      case Joystick::ButtonSymbolSmallLeftTriangle:
         drawSmallLeftTriangle(center, labelColor);
         break;
      case Joystick::ButtonSymbolSmallRightTriangle:
         drawSmallRightTriangle(center, labelColor);
         break;
      case Joystick::ButtonSymbolNone:
      default:
         RenderUtils::drawString_fixed(location.x - RenderUtils::getStringWidth(labelSize, label) / 2, location.y + 2, labelSize, labelColor, label);
         break;
   }

   return true;
}


void JoystickRender::drawPlaystationCross(const Point &center)
{
   Point p1(center + Point(-5, -5));
   Point p2(center + Point(5, 5));
   Point p3(center + Point(-5, 5));
   Point p4(center + Point(5, -5));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y,
         p4.x, p4.y
   };
   RenderUtils::drawLines(vertices, ARRAYSIZE(vertices) / 2, Colors::paleBlue);
}


void JoystickRender::drawPlaystationCircle(const Point &center)
{
   RenderUtils::drawCircle(center, 6, Colors::paleRed);
}


void JoystickRender::drawPlaystationSquare(const Point &center)
{
   F32 vertices[] = {
      center.x - 5, center.y - 5,
      center.x - 5, center.y + 5,
      center.x + 5, center.y + 5,
      center.x + 5, center.y - 5,
   };

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, Colors::palePurple);   // ARRAYSIZE = 8, have 4 points
}


void JoystickRender::drawPlaystationTriangle(const Point &center, const Color &color)
{
   F32 vertices[] = {
      center.x,     center.y - 7,
      center.x + 6, center.y + 5,
      center.x - 6, center.y + 5
   };

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, color);
}


void JoystickRender::drawSmallLeftTriangle(const Point &center, const Color &color)
{
   F32 vertices[] = {
         center.x + 4, center.y,
         center.x - 3, center.y + 5,
         center.x - 3, center.y - 5
   };

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, color);
}


void JoystickRender::drawSmallRightTriangle(const Point &center, const Color &color)
{
   F32 vertices[] = {
         center.x - 4, center.y,
         center.x + 3, center.y + 5,
         center.x + 3, center.y - 5
   };

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, color);
}


void JoystickRender::drawButtonRightTriangle(const Point &center, const Color &color)
{
   F32 vertices[] = {
         center.x - 15, center.y -  9,
         center.x - 15, center.y + 10,
         center.x + 12, center.y
   };

   RenderUtils::drawLineLoop(vertices, ARRAYSIZE(vertices) / 2, color);
}


static Point points[7];    // Reuse this for the items below

void JoystickRender::drawDPadUp(const Point &center, const Color &color)
{
   // Up arrow
   points[0] = (center + Point(-3, 6));
   points[1] = (center + Point(-3, 0));
   points[2] = (center + Point(-9, 0));
   points[3] = (center + Point( 0,-9));
   points[4] = (center + Point( 9, 0));
   points[5] = (center + Point( 3, 0));
   points[6] = (center + Point( 3, 6));

   RenderUtils::drawLineLoop((F32 *)points, ARRAYSIZE(points), color);
}


void JoystickRender::drawDPadDown(const Point &center, const Color &color)
{
   // Down arrow
   points[0] = (center + Point(-3,-6));
   points[1] = (center + Point(-3, 0));
   points[2] = (center + Point(-9, 0));
   points[3] = (center + Point( 0, 9));
   points[4] = (center + Point( 9, 0));
   points[5] = (center + Point( 3, 0));
   points[6] = (center + Point( 3,-6));

   RenderUtils::drawLineLoop((F32 *)points, ARRAYSIZE(points), color);
}


void JoystickRender::drawDPadLeft(const Point &center, const Color &color)
{
   // Left arrow
   points[0] = (center + Point( 6,-3));
   points[1] = (center + Point( 0,-3));
   points[2] = (center + Point( 0,-9));
   points[3] = (center + Point(-9, 0));
   points[4] = (center + Point( 0, 9));
   points[5] = (center + Point( 0, 3));
   points[6] = (center + Point( 6, 3));

   RenderUtils::drawLineLoop((F32 *)points, ARRAYSIZE(points), color);
}


void JoystickRender::drawDPadRight(const Point &center, const Color &color)
{
   // Right arrow
   points[0] = (center + Point(-6,-3));
   points[1] = (center + Point( 0,-3));
   points[2] = (center + Point( 0,-9));
   points[3] = (center + Point( 9, 0));
   points[4] = (center + Point( 0, 9));
   points[5] = (center + Point( 0, 3));
   points[6] = (center + Point(-6, 3));

   RenderUtils::drawLineLoop((F32 *)points, ARRAYSIZE(points), color);
}

////////// End rendering functions

} /* namespace Zap */
