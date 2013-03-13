//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "JoystickRender.h"
#include "UI.h"
#include "gameObjectRender.h"
#include "config.h"
#include "Colors.h"

#include "OpenglUtils.h"

namespace Zap
{

JoystickRender::JoystickRender()
{
}

JoystickRender::~JoystickRender()
{
}

inline void JoystickRender::setButtonColor(bool activated)
{
   if(activated)
      glColor(Colors::red);
   else
      glColor(Colors::white);
}


// Render dpad graphic
void JoystickRender::renderDPad(Point center, F32 radius, bool upActivated, bool downActivated, bool leftActivated,
                bool rightActivated, const char *msg1, const char *msg2)
{
   radius = radius * 0.143f;   // = 1/7  Correct for the fact that when radius = 1, graphic has 7 px radius

   // Reusable
   Point point1;
   Point point2;
   Point point3;
   Point point4;
   Point point5;
   Point point6;
   Point point7;

   // Up arrow
   setButtonColor(upActivated);
   point1 = (center + Point(-1, -2) * radius);
   point2 = (center + Point(-1, -4) * radius);
   point3 = (center + Point(-3, -4) * radius);
   point4 = (center + Point(0, -7) * radius);
   point5 = (center + Point(3, -4) * radius);
   point6 = (center + Point(1, -4) * radius);
   point7 = (center + Point(1, -2) * radius);
   F32 vertices[] = {
         point1.x, point1.y,
         point2.x, point2.y,
         point3.x, point3.y,
         point4.x, point4.y,
         point5.x, point5.y,
         point6.x, point6.y,
         point7.x, point7.y
   };
   renderVertexArray(vertices, 7, GL_LINE_LOOP);

   // Down arrow
   setButtonColor(downActivated);
   point1 = (center + Point(-1, 2) * radius);
   point2 = (center + Point(-1, 4) * radius);
   point3 = (center + Point(-3, 4) * radius);
   point4 = (center + Point(0, 7) * radius);
   point5 = (center + Point(3, 4) * radius);
   point6 = (center + Point(1, 4) * radius);
   point7 = (center + Point(1, 2) * radius);
   F32 vertices2[] = {
         point1.x, point1.y,
         point2.x, point2.y,
         point3.x, point3.y,
         point4.x, point4.y,
         point5.x, point5.y,
         point6.x, point6.y,
         point7.x, point7.y
   };
   renderVertexArray(vertices2, 7, GL_LINE_LOOP);

   // Left arrow
   setButtonColor(leftActivated);
   point1 = (center + Point(-2, -1) * radius);
   point2 = (center + Point(-4, -1) * radius);
   point3 = (center + Point(-4, -3) * radius);
   point4 = (center + Point(-7, 0) * radius);
   point5 = (center + Point(-4, 3) * radius);
   point6 = (center + Point(-4, 1) * radius);
   point7 = (center + Point(-2, 1) * radius);
   F32 vertices3[] = {
         point1.x, point1.y,
         point2.x, point2.y,
         point3.x, point3.y,
         point4.x, point4.y,
         point5.x, point5.y,
         point6.x, point6.y,
         point7.x, point7.y
   };
   renderVertexArray(vertices3, 7, GL_LINE_LOOP);

   // Right arrow
   setButtonColor(rightActivated);
   point1 = (center + Point(2, -1) * radius);
   point2 = (center + Point(4, -1) * radius);
   point3 = (center + Point(4, -3) * radius);
   point4 = (center + Point(7, 0) * radius);
   point5 = (center + Point(4, 3) * radius);
   point6 = (center + Point(4, 1) * radius);
   point7 = (center + Point(2, 1) * radius);
   F32 vertices4[] = {
         point1.x, point1.y,
         point2.x, point2.y,
         point3.x, point3.y,
         point4.x, point4.y,
         point5.x, point5.y,
         point6.x, point6.y,
         point7.x, point7.y
   };
   renderVertexArray(vertices4, 7, GL_LINE_LOOP);

   // Label the graphic
   glColor(1, 1, 1);
   if(strcmp(msg1, ""))    // That is, != "".  Remember, kids, strcmp returns 0 when strings are identical!
   {
      S32 size = 12;
      S32 width = getStringWidth(size, msg1);
      drawString(center.x - width / 2, center.y + 27, size, msg1);
   }

   if(strcmp(msg2, ""))
   {
      S32 size = 10;
      S32 width = getStringWidth(size, msg2);
      drawString(center.x - width / 2, center.y + 42, size, msg2);
   }
}


extern Joystick::Button inputCodeToJoystickButton(InputCode inputCode);


S32 JoystickRender::getControllerButtonRenderedSize(S32 joystickIndex, InputCode inputCode)
{
   // Return keyboard key size, just in case
   if(!InputCodeManager::isControllerButton(inputCode))
      return getStringWidthf(15, "[%s]", InputCodeManager::inputCodeToString(inputCode));

   // Get joystick button size
   Joystick::Button button = inputCodeToJoystickButton(inputCode);

   Joystick::ButtonShape buttonShape =
         Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonShape;

   switch(buttonShape)
   {
      case Joystick::ButtonShapeRound:
         return buttonHalfHeight * 2;

      case Joystick::ButtonShapeRect:
         return rectButtonWidth;

      case Joystick::ButtonShapeSmallRect:
         return smallRectButtonWidth;

      case Joystick::ButtonShapeRoundedRect:
         return rectButtonWidth;

      case Joystick::ButtonShapeSmallRoundedRect:
         return smallRectButtonWidth;

      case Joystick::ButtonShapeHorizEllipse:
         return horizEllipseButtonRadiusX * 2;

      case Joystick::ButtonShapeRightTriangle:
         return rectButtonWidth;

      default:
         return rectButtonWidth;
   }

   return -1;     // Kill a useless warning
}


// Renders something resembling a controller button or keyboard key
// Note:  buttons are with the given x coordinate as their center
void JoystickRender::renderControllerButton(F32 x, F32 y, U32 joystickIndex, InputCode inputCode, bool activated)
{
   // Render keyboard keys, just in case
   if(!InputCodeManager::isControllerButton(inputCode))
   {
      // Offset a bit in the x direction.
      drawStringf(x - 10, y, 15, "[%s]", InputCodeManager::inputCodeToString(inputCode));
      return;
   }

   Joystick::Button button = inputCodeToJoystickButton(inputCode);

   // Don't render if button doesn't exist
   if(Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].sdlButton == Joystick::FakeRawButton)
      return;

   Joystick::ButtonShape buttonShape =
         Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonShape;

   const char *label = Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].label.c_str();
   Color *buttonColor = &Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].color;


   // Note:  the x coordinate is already at the center
   Point location(x,y);
   Point center = location + Point(0, buttonHalfHeight);

   // Change button outline color if activated
   setButtonColor(activated);

   // Render joystick button shape
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
         glColor(buttonColor);
         drawFilledEllipse(center, horizEllipseButtonRadiusX, horizEllipseButtonRadiusY, 0);
         glColor(Colors::white);
         drawEllipse(center, horizEllipseButtonRadiusX, horizEllipseButtonRadiusY, 0);
         break;
      case Joystick::ButtonShapeRightTriangle:
         location = location + Point(-rightTriangleWidth/4, 0);  // Need to off-center the label slightly for this button
         drawButtonRightTriangle(center);
         break;
      case Joystick::ButtonShapeRound:
      default:
         drawCircle(center, (F32)buttonHalfHeight);
         break;
   }

   // Now render joystick label or symbol
   Joystick::ButtonSymbol buttonSymbol =
         Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonSymbol;

   // Change color of label to the preset (default white)
   glColor(buttonColor);

   switch(buttonSymbol)
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
   renderVertexArray(vertices, 4, GL_LINES);
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
   renderVertexArray(vertices, 4, GL_LINE_LOOP);
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
   renderVertexArray(vertices, 3, GL_LINE_LOOP);
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
   renderVertexArray(vertices, 3, GL_LINE_LOOP);
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
   renderVertexArray(vertices, 3, GL_LINE_LOOP);
}


void JoystickRender::drawButtonRightTriangle(const Point & center)
{
   Point p1(center + Point(-15, -9));
   Point p2(center + Point(-15, 10));
   Point p3(center + Point(12, 0));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, 3, GL_LINE_LOOP);
}

////////// End rendering functions

} /* namespace Zap */
