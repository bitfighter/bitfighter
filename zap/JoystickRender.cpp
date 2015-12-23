//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "JoystickRender.h"
#include "Joystick.h"
#include "InputCode.h"
#include "SymbolShape.h"
#include "FontManager.h"
#include "Colors.h"
#include "gameObjectRender.h"
#include "SymbolShape.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"

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


inline const Color *JoystickRender::getButtonColor(bool activated)
{
   if(activated)
      return &Colors::red;

   return &Colors::white;
}


// Render dpad graphic
void JoystickRender::renderDPad(Point center, bool upActivated, bool downActivated, bool leftActivated,
                bool rightActivated, const char *msg1, const char *msg2)
{
   glColor(getButtonColor(upActivated));
   drawDPadUp(center + Point(0,-16));

   glColor(getButtonColor(downActivated));
   drawDPadDown(center + Point(0,16));

   glColor(getButtonColor(leftActivated));
   drawDPadLeft(center + Point(-16,0));

   glColor(getButtonColor(rightActivated));
   drawDPadRight(center + Point(16,0));

   // Label the graphic
   glColor(Colors::white);
   if(strcmp(msg1, "") == 0)    // That is, != "".  Remember, kids, strcmp returns 0 when strings are identical!
   {
      S32 size = 12;
      S32 width = getStringWidth(size, msg1);
      drawString(center.x - width / 2, center.y + 27, size, msg1);
   }

   if(strcmp(msg2, "") == 0)
   {
      S32 size = 10;
      S32 width = getStringWidth(size, msg2);
      drawString(center.x - width / 2, center.y + 42, size, msg2);
   }
}

static S32 SymbolPadding = 6;       // Just some padding we throw around our symbols to make them look hot

S32 JoystickRender::getControllerButtonRenderedSize(InputCode inputCode)
{
   // Return keyboard key size, just in case
   if(!InputCodeManager::isControllerButton(inputCode))
      return SymbolKey(InputCodeManager::inputCodeToString(inputCode)).getWidth();

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

   return -1;     // Kill a useless warning -- can never get here!
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
// Note:  buttons are with the given x coordinate as their _center_
bool JoystickRender::renderControllerButton(F32 centerX, F32 centerY, InputCode inputCode, const Color *overrideRenderColor)
{
   // Set the basic color, could be overridden later
   if(overrideRenderColor)
      glColor(overrideRenderColor);
   else
      glColor(Colors::white);


   // Render keyboard keys, just in case
   if(!InputCodeManager::isControllerButton(inputCode))
   {
      SymbolKey(InputCodeManager::inputCodeToString(inputCode)).render(centerX, centerY + 17, AlignmentCenter);
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

   // Note:  the x coordinate is already at the center
   Point location(centerX, centerY);
   Point center = location + Point(0, buttonHalfHeight);

   /////
   // 1. Render joystick button shape
   switch(buttonShape)
   {
      case Joystick::ButtonShapeRect:
         drawRoundedRect(center, rectButtonWidth, rectButtonHeight, 3);
         break;
      case Joystick::ButtonShapeSmallRect:
         drawRoundedRect(center, smallRectButtonWidth, smallRectButtonHeight, 3);
         break;
      case Joystick::ButtonShapeRoundedRect:
         drawRoundedRect(center, rectButtonWidth, rectButtonHeight, 5);
         break;
      case Joystick::ButtonShapeSmallRoundedRect:
         drawRoundedRect(center, smallRectButtonWidth, smallRectButtonHeight, 5);
         break;
      case Joystick::ButtonShapeHorizEllipse:
         drawEllipse(center, horizEllipseButtonRadiusX, horizEllipseButtonRadiusY, 0);
         break;
      case Joystick::ButtonShapeRightTriangle:
         location = location + Point(-rightTriangleWidth / 4.0f, 0);  // Need to off-center the label slightly for this button
         drawButtonRightTriangle(center);
         break;

      // DPad
      case Joystick::ButtonShapeDPadUp:
         JoystickRender::drawDPadUp(center);
         break;
      case Joystick::ButtonShapeDPadDown:
         JoystickRender::drawDPadDown(center);
         break;
      case Joystick::ButtonShapeDPadLeft:
         JoystickRender::drawDPadLeft(center);
         break;
      case Joystick::ButtonShapeDPadRight:
         JoystickRender::drawDPadRight(center);
         break;

      case Joystick::ButtonShapeRound:
      default:
         //shapeCircle.render(center);
         drawCircle(center, buttonHalfHeight);
         break;
   }

   /////
   // 2. Render joystick label or symbol

   // Change color of label to the preset (default white)
   if(overrideRenderColor)
      glColor(overrideRenderColor);
   else
      glColor(buttonColor);


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
         drawPlaystationTriangle(center);
         break;
      case Joystick::ButtonSymbolSmallLeftTriangle:
         drawSmallLeftTriangle(center);
         break;
      case Joystick::ButtonSymbolSmallRightTriangle:
         drawSmallRightTriangle(center);
         break;
      case Joystick::ButtonSymbolNone:
      default:
         drawString(location.x - getStringWidth(labelSize, label) / 2, location.y + 2, labelSize, label);
         break;
   }

   return true;
}


void JoystickRender::drawPlaystationCross(const Point &center)
{
   glColor(Colors::paleBlue);
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
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);
}


void JoystickRender::drawPlaystationCircle(const Point &center)
{
   glColor(Colors::paleRed);
   drawCircle(center, 6);
}


void JoystickRender::drawPlaystationSquare(const Point &center)
{
   glColor(Colors::palePurple);
   Point p1(center + Point(-5, -5));
   Point p2(center + Point(-5, 5));
   Point p3(center + Point(5, 5));
   Point p4(center + Point(5, -5));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y,
         p4.x, p4.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void JoystickRender::drawPlaystationTriangle(const Point &center)
{
   Point p1(center + Point(0, -7));
   Point p2(center + Point(6, 5));
   Point p3(center + Point(-6, 5));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void JoystickRender::drawSmallLeftTriangle(const Point & center)
{
   Point p1(center + Point(4, 0));
   Point p2(center + Point(-3, 5));
   Point p3(center + Point(-3, -5));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void JoystickRender::drawSmallRightTriangle(const Point & center)
{
   Point p1(center + Point(-4, 0));
   Point p2(center + Point(3, 5));
   Point p3(center + Point(3, -5));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void JoystickRender::drawButtonRightTriangle(const Point &center)
{
   Point p1(center + Point(-15, -9));
   Point p2(center + Point(-15, 10));
   Point p3(center + Point(12, 0));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void JoystickRender::drawDPadUp(Point center)
{
   static Point points[7];

   // Up arrow
   points[0] = (center + Point(-3, 6));
   points[1] = (center + Point(-3, 0));
   points[2] = (center + Point(-9, 0));
   points[3] = (center + Point( 0,-9));
   points[4] = (center + Point( 9, 0));
   points[5] = (center + Point( 3, 0));
   points[6] = (center + Point( 3, 6));

   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);
}


void JoystickRender::drawDPadDown(Point center)
{
   static Point points[7];

   // Down arrow
   points[0] = (center + Point(-3,-6));
   points[1] = (center + Point(-3, 0));
   points[2] = (center + Point(-9, 0));
   points[3] = (center + Point( 0, 9));
   points[4] = (center + Point( 9, 0));
   points[5] = (center + Point( 3, 0));
   points[6] = (center + Point( 3,-6));

   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);
}


void JoystickRender::drawDPadLeft(Point center)
{
   static Point points[7];

   // Left arrow
   points[0] = (center + Point( 6,-3));
   points[1] = (center + Point( 0,-3));
   points[2] = (center + Point( 0,-9));
   points[3] = (center + Point(-9, 0));
   points[4] = (center + Point( 0, 9));
   points[5] = (center + Point( 0, 3));
   points[6] = (center + Point( 6, 3));

   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);
}


void JoystickRender::drawDPadRight(Point center)
{
   static Point points[7];

   // Right arrow
   points[0] = (center + Point(-6,-3));
   points[1] = (center + Point( 0,-3));
   points[2] = (center + Point( 0,-9));
   points[3] = (center + Point( 9, 0));
   points[4] = (center + Point( 0, 9));
   points[5] = (center + Point( 0, 3));
   points[6] = (center + Point(-6, 3));

   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);
}

////////// End rendering functions

} /* namespace Zap */
