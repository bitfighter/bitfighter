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
void JoystickRender::renderDPad(Point center, F32 radius, bool upActivated, bool downActivated, bool leftActivated,
                bool rightActivated, const char *msg1, const char *msg2)
{
   radius = radius * 0.143f;   // = 1/7  Correct for the fact that when radius = 1, graphic has 7 px radius

   static Point points[7];

   // Up arrow
   points[0] = (center + Point(-1, -2) * radius);
   points[1] = (center + Point(-1, -4) * radius);
   points[2] = (center + Point(-3, -4) * radius);
   points[3] = (center + Point( 0, -7) * radius);
   points[4] = (center + Point( 3, -4) * radius);
   points[5] = (center + Point( 1, -4) * radius);
   points[6] = (center + Point( 1, -2) * radius);

   glColor(getButtonColor(upActivated));
   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);

   // Down arrow
   points[0] = (center + Point(-1, 2) * radius);
   points[1] = (center + Point(-1, 4) * radius);
   points[2] = (center + Point(-3, 4) * radius);
   points[3] = (center + Point( 0, 7) * radius);
   points[4] = (center + Point( 3, 4) * radius);
   points[5] = (center + Point( 1, 4) * radius);
   points[6] = (center + Point( 1, 2) * radius);

   glColor(getButtonColor(downActivated));
   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);

   // Left arrow
   points[0] = (center + Point(-2, -1) * radius);
   points[1] = (center + Point(-4, -1) * radius);
   points[2] = (center + Point(-4, -3) * radius);
   points[3] = (center + Point(-7,  0) * radius);
   points[4] = (center + Point(-4,  3) * radius);
   points[5] = (center + Point(-4,  1) * radius);
   points[6] = (center + Point(-2,  1) * radius);

   glColor(getButtonColor(leftActivated));
   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);

   // Right arrow
   points[0] = (center + Point(2, -1) * radius);
   points[1] = (center + Point(4, -1) * radius);
   points[2] = (center + Point(4, -3) * radius);
   points[3] = (center + Point(7,  0) * radius);
   points[4] = (center + Point(4,  3) * radius);
   points[5] = (center + Point(4,  1) * radius);
   points[6] = (center + Point(2,  1) * radius);

   glColor(getButtonColor(rightActivated));
   renderVertexArray((F32 *)points, ARRAYSIZE(points), GL_LINE_LOOP);

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
   JoystickButton button = InputCodeManager::inputCodeToJoystickButton(inputCode);
   S32 joystickIndex = Joystick::SelectedPresetIndex;

   Joystick::ButtonShape buttonShape =
         Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonShape;

   switch(buttonShape)
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
bool JoystickRender::renderControllerButton(F32 centerX, F32 centerY, U32 joystickIndex, InputCode inputCode, const Color *overrideRenderColor)
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

   JoystickButton button = InputCodeManager::inputCodeToJoystickButton(inputCode);

   // Don't render if button doesn't exist
   if(!Joystick::isButtonDefined(joystickIndex, button))
      return false;

   Joystick::ButtonShape buttonShape =
         Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonShape;

   const char *label = Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].label.c_str();
   const Color *buttonColor = &Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].color;

   // Note:  the x coordinate is already at the center
   Point location(centerX, centerY);
   Point center = location + Point(0, buttonHalfHeight);

   /////
   // 1. Render joystick button shape
   switch(buttonShape)
   {
      case Joystick::ButtonShapeRect:
         //shapeRect.render(center);
         drawRoundedRect(center, rectButtonWidth, rectButtonHeight, 3);
         break;
      case Joystick::ButtonShapeSmallRect:
         //shapeSmallRect.render(center);
         drawRoundedRect(center, smallRectButtonWidth, smallRectButtonHeight, 3);
         break;
      case Joystick::ButtonShapeRoundedRect:
         //shapeRoundedRect.render(center);
         drawRoundedRect(center, rectButtonWidth, rectButtonHeight, 5);
         break;
      case Joystick::ButtonShapeSmallRoundedRect:
         //shapeSmallRoundedRect.render(center);
         drawRoundedRect(center, smallRectButtonWidth, smallRectButtonHeight, 5);
         break;
      case Joystick::ButtonShapeHorizEllipse:
         if(!overrideRenderColor)
            glColor(buttonColor);
         //shapeHorizEllipse.render(center);
         drawFilledEllipse(center, horizEllipseButtonRadiusX, horizEllipseButtonRadiusY, 0);
         glColor(Colors::white);
         drawEllipse(center, horizEllipseButtonRadiusX, horizEllipseButtonRadiusY, 0);
         break;
      case Joystick::ButtonShapeRightTriangle:
         //shapeRightTriangle.render(center);
         location = location + Point(-rightTriangleWidth / 4.0f, 0);  // Need to off-center the label slightly for this button
         drawButtonRightTriangle(center);
         break;
      case Joystick::ButtonShapeRound:
      default:
         //shapeCircle.render(center);
         drawCircle(center, (F32)buttonHalfHeight);
         break;
   }

   /////
   // 2. Render joystick label or symbol

   // Change color of label to the preset (default white)
   if(overrideRenderColor)
      glColor(overrideRenderColor);
   else
      glColor(buttonColor);


   switch(Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonSymbol)
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

////////// End rendering functions

} /* namespace Zap */
