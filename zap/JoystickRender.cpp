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

#include "SDL/SDL_opengl.h"

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

   // Up arrow
   setButtonColor(upActivated);
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(-1, -2) * radius);
      glVertex(center + Point(-1, -4) * radius);
      glVertex(center + Point(-3, -4) * radius);
      glVertex(center + Point(0, -7) * radius);
      glVertex(center + Point(3, -4) * radius);
      glVertex(center + Point(1, -4) * radius);
      glVertex(center + Point(1, -2) * radius);
   glEnd();

   // Down arrow
   setButtonColor(downActivated);
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(-1, 2) * radius);
      glVertex(center + Point(-1, 4) * radius);
      glVertex(center + Point(-3, 4) * radius);
      glVertex(center + Point(0, 7) * radius);
      glVertex(center + Point(3, 4) * radius);
      glVertex(center + Point(1, 4) * radius);
      glVertex(center + Point(1, 2) * radius);
   glEnd();

   // Left arrow
   setButtonColor(leftActivated);
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(-2, -1) * radius);
      glVertex(center + Point(-4, -1) * radius);
      glVertex(center + Point(-4, -3) * radius);
      glVertex(center + Point(-7, 0) * radius);
      glVertex(center + Point(-4, 3) * radius);
      glVertex(center + Point(-4, 1) * radius);
      glVertex(center + Point(-2, 1) * radius);
   glEnd();

   // Right arrow
   setButtonColor(rightActivated);
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(2, -1) * radius);
      glVertex(center + Point(4, -1) * radius);
      glVertex(center + Point(4, -3) * radius);
      glVertex(center + Point(7, 0) * radius);
      glVertex(center + Point(4, 3) * radius);
      glVertex(center + Point(4, 1) * radius);
      glVertex(center + Point(2, 1) * radius);
   glEnd();

   // Label the graphic
   glColor3f(1, 1, 1);
   if(strcmp(msg1, ""))    // That is, != "".  Remember, kids, strcmp returns 0 when strings are identical!
   {
      S32 size = 12;
      S32 width = UserInterface::getStringWidth(size, msg1);
      UserInterface::drawString(center.x - width / 2, center.y + 27, size, msg1);
   }

   if(strcmp(msg2, ""))
   {
      S32 size = 10;
      S32 width = UserInterface::getStringWidth(size, msg2);
      UserInterface::drawString(center.x - width / 2, center.y + 42, size, msg2);
   }
}


extern Joystick::Button inputCodeToJoystickButton(InputCode inputCode);


S32 JoystickRender::getControllerButtonRenderedSize(S32 joystickIndex, InputCode inputCode)
{
   // Return keyboard key size, just in case
   if(!isControllerButton(inputCode))
      return UserInterface::getStringWidthf(15, "[%s]", inputCodeToString(inputCode));

   // Get joystick button size
   Joystick::Button button = inputCodeToJoystickButton(inputCode);

   Joystick::ButtonShape buttonShape =
         Joystick::JoystickPresetList[joystickIndex].buttonMappings[button].buttonShape;

   switch(buttonShape)
   {
      case Joystick::ButtonShapeRound:
         return buttonHalfHeight * 2;
         break;
      case Joystick::ButtonShapeRect:
         return rectButtonWidth;
         break;
      case Joystick::ButtonShapeSmallRect:
         return smallRectButtonWidth;
         break;
      case Joystick::ButtonShapeRoundedRect:
         return rectButtonWidth;
         break;
      case Joystick::ButtonShapeSmallRoundedRect:
         return smallRectButtonWidth;
         break;
      case Joystick::ButtonShapeHorizEllipse:
         return horizEllipseButtonRadiusX;
         break;
      case Joystick::ButtonShapeRightTriangle:
         return rectButtonWidth;
         break;
      default:
         return rectButtonWidth;
   }

   return -1;     // Kill a useless warning
}


// Renders something resembling a controller button or keyboard key
void JoystickRender::renderControllerButton(F32 x, F32 y, U32 joystickIndex, InputCode inputCode, bool activated, S32 offset)
{
   // Render keyboard keys, just in case
   if(!isControllerButton(inputCode))
   {
      UserInterface::drawStringf(x, y, 15, "[%s]", inputCodeToString(inputCode));
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
         glBegin(GL_LINE_LOOP);
            glVertex(center + Point(-15, -9));
            glVertex(center + Point(-15, 10));
            glVertex(center + Point(12, 0));
         glEnd();
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
         UserInterface::drawString(location.x - UserInterface::getStringWidth(labelSize, label) / 2, location.y + 2, labelSize, label);
         break;
   }
}


void JoystickRender::drawPlaystationCross(const Point &center)
{
   glColor(Colors::paleBlue);
   glBegin(GL_LINES);
      glVertex(center + Point(-5, -5));
      glVertex(center + Point(5, 5));
      glVertex(center + Point(-5, 5));
      glVertex(center + Point(5, -5));
   glEnd();
}


void JoystickRender::drawPlaystationCircle(const Point &center)
{
   glColor(Colors::paleRed);
   drawCircle(center, 6);
}


void JoystickRender::drawPlaystationSquare(const Point &center)
{
   glColor(Colors::palePurple);
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(-5, -5));
      glVertex(center + Point(-5, 5));
      glVertex(center + Point(5, 5));
      glVertex(center + Point(5, -5));
   glEnd();
}


void JoystickRender::drawPlaystationTriangle(const Point &center)
{
   glColor(Colors::paleGreen);
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(0, -7));
      glVertex(center + Point(6, 5));
      glVertex(center + Point(-6, 5));
   glEnd();
}


void JoystickRender::drawSmallLeftTriangle(const Point & center)
{
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(4, 0));
      glVertex(center + Point(-3, 5));
      glVertex(center + Point(-3, -5));
   glEnd();
}


void JoystickRender::drawSmallRightTriangle(const Point & center)
{
   glBegin(GL_LINE_LOOP);
      glVertex(center + Point(-4, 0));
      glVertex(center + Point(3, 5));
      glVertex(center + Point(3, -5));
   glEnd();
}

////////// End rendering functions

} /* namespace Zap */
