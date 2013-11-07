//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef JOYSTICKRENDER_H_
#define JOYSTICKRENDER_H_

#include "tnlTypes.h"
#include "Point.h"
#include "InputCodeEnum.h"

using namespace TNL;

namespace Zap
{

class Color;

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


   static bool renderControllerButton(F32 centerX, F32 centerY, U32 joystickIndex, InputCode inputCode, const Color *overrideRenderColor = NULL);
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

   static inline const Color *getButtonColor(bool activated);
};

} /* namespace Zap */

#endif /* JOYSTICKRENDER_H_ */
