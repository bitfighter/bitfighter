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

#ifndef JOYSTICKRENDER_H_
#define JOYSTICKRENDER_H_

#include "tnlTypes.h"
#include "InputCode.h"
#include "Point.h"
#include "Joystick.h"

using namespace TNL;

namespace Zap
{


class JoystickRender
{
private:
   static const S32 buttonHalfHeight = 9;   // This is the default half-height of a button
   static const S32 rectButtonWidth = 24;
   static const S32 rectButtonHeight = 18;
   static const S32 smallRectButtonWidth = 19;
   static const S32 smallRectButtonHeight = 15;
   static const S32 horizEllipseButtonRadiusX = 14;
   static const S32 horizEllipseButtonRadiusY = 8;
   static const S32 rightTriangleWidth = 28;
   static const S32 rightTriangleHeight = 18;

   static const S32 labelSize = 12;

public:

   JoystickRender();
   virtual ~JoystickRender();


   static void renderControllerButton(F32 x, F32 y, U32 joystickIndex, InputCode inputCode, bool activated);
   static S32 getControllerButtonRenderedSize(S32 joystickIndex, InputCode inputCode);

   static void renderDPad(Point center, F32 radius, bool upActivated, bool downActivated, bool leftActivated,
         bool rightActivated, const char *msg1, const char *msg2);

   static void drawPlaystationCross(const Point &center);
   static void drawPlaystationCircle(const Point &center);
   static void drawPlaystationSquare(const Point &center);
   static void drawPlaystationTriangle(const Point &center);
   static void drawSmallLeftTriangle(const Point & center);
   static void drawSmallRightTriangle(const Point & center);
   static void drawButtonRightTriangle(const Point & center);

   static inline void setButtonColor(bool activated);
};

} /* namespace Zap */

#endif /* JOYSTICKRENDER_H_ */
