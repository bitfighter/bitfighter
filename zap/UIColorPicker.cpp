//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
#include "UIColorPicker.h"

#include "GameObjectRender.h"
#include "UIManager.h"
#include "FontManager.h"
#include "Cursor.h"
#include "Colors.h"
#include "ship.h"

#include "RenderUtils.h"
#include "GeomUtils.h"    // For triangulation

namespace Zap
{

static void limitRange(Color &c)
{
   if(c.r < 0) c.r = 0;
   if(c.r > 1) c.r = 1;

   if(c.g < 0) c.g = 0;
   if(c.g > 1) c.g = 1;

   if(c.b < 0) c.b = 0;
   if(c.b > 1) c.b = 1;
}

const S32 colorWheel_x = 100;
const S32 colorWheel_y = 100;
const S32 colorWheel_w = 400;
const S32 colorWheel_h = 400;
const S32 colorWheel_rad = 400;

const S32 colorBrightness_x = 550;
const S32 colorBrightness_y = 100;
const S32 colorBrightness_w = 25;
const S32 colorBrightness_w_space = 50;
const S32 colorBrightness_h = 400;

UIColorPicker::UIColorPicker(ClientGame *game, UIManager *uiManager) :
   Parent(game, uiManager) 
{
   mMouseDown = 0;
}


UIColorPicker::~UIColorPicker()
{ 
   // Do nothing
}


void UIColorPicker::onActivate()
{
   Parent::onActivate();
   mMouseDown = 0;
   Cursor::enableCursor();
}


void UIColorPicker::onReactivate()
{
   Parent::onReactivate();
   mMouseDown = 0;
   Cursor::enableCursor();
}


void UIColorPicker::idle(U32 timeDelta) { Parent::idle(timeDelta); }


const F32 colorWheelPoints[] = {
   colorWheel_w / 2 + colorWheel_x, colorWheel_h / 2     + colorWheel_y,  // center
                      colorWheel_x, colorWheel_h / 4     + colorWheel_y,  // top-left
                      colorWheel_x, colorWheel_h * 3 / 4 + colorWheel_y,  // bottom-left
   colorWheel_w / 2 + colorWheel_x, colorWheel_h         + colorWheel_y,  // bottom
   colorWheel_w     + colorWheel_x, colorWheel_h * 3 / 4 + colorWheel_y,  // bottom-right
   colorWheel_w     + colorWheel_x, colorWheel_h / 4     + colorWheel_y,  // top-right
   colorWheel_w / 2 + colorWheel_x,                        colorWheel_y,  // top
                      colorWheel_x, colorWheel_h / 4     + colorWheel_y,  // top-left
};


const F32 colorBrightnessPoints[] = {
   colorBrightness_x,                     colorBrightness_y,
   colorBrightness_x + colorBrightness_w, colorBrightness_y,
   colorBrightness_x + colorBrightness_w, colorBrightness_y + colorBrightness_h,
   colorBrightness_x,                     colorBrightness_y + colorBrightness_h,
};


const F32 colorBrightnessPointsRed[] = {
   colorBrightness_x + colorBrightness_w_space,                     colorBrightness_y,
   colorBrightness_x + colorBrightness_w_space + colorBrightness_w, colorBrightness_y,
   colorBrightness_x + colorBrightness_w_space + colorBrightness_w, colorBrightness_y + colorBrightness_h,
   colorBrightness_x + colorBrightness_w_space,                     colorBrightness_y + colorBrightness_h,
};


const F32 colorBrightnessPointsGreen[] = {
   colorBrightness_x + colorBrightness_w_space * 2,                     colorBrightness_y,
   colorBrightness_x + colorBrightness_w_space * 2 + colorBrightness_w, colorBrightness_y,
   colorBrightness_x + colorBrightness_w_space * 2 + colorBrightness_w, colorBrightness_y + colorBrightness_h,
   colorBrightness_x + colorBrightness_w_space * 2,                     colorBrightness_y + colorBrightness_h,
};


const F32 colorBrightnessPointsBlue[] = {
   colorBrightness_x + colorBrightness_w_space * 3,                     colorBrightness_y,
   colorBrightness_x + colorBrightness_w_space * 3 + colorBrightness_w, colorBrightness_y,
   colorBrightness_x + colorBrightness_w_space * 3 + colorBrightness_w, colorBrightness_y + colorBrightness_h,
   colorBrightness_x + colorBrightness_w_space * 3,                     colorBrightness_y + colorBrightness_h,
};


void UIColorPicker::drawArrow(F32 *p, const Color &color)
{
   p[2] = p[0] + 20; p[3] = p[1] - 10;
   p[4] = p[0] + 20; p[5] = p[1] + 10;
   RenderUtils::drawLineLoop(p, 3, color);
}


void UIColorPicker::render() const
{
   FontManager::pushFontContext(MenuHeaderContext);
   RenderUtils::drawCenteredUnderlinedString(15, 30, Colors::green, "COLOR PICKER");
   RenderUtils::drawStringc     (400, 580, 30, Colors::green, "Done");
   RenderUtils::drawString_fixed(730, 595, 15, Colors::green, "Cancel");
   FontManager::popFontContext();


   F32 maxCol = max(r, g);

   if(b > maxCol)
      maxCol = b;

   F32 r2, g2, b2;

   if(maxCol == 0)
      b2 = g2 = r2 = 1;
   else
   {
      r2 = r / maxCol;
      g2 = g / maxCol;
      b2 = b / maxCol;
   }

   // OLD COLOR WHEEL
   // Color array aligned with colorWheelPoints
//   F32 colorArray[32] = {
//      maxCol, maxCol ,maxCol, 1,
//      maxCol,      0,      0, 1,
//      maxCol,      0, maxCol, 1,
//           0,      0, maxCol, 1,
//           0, maxCol, maxCol, 1,
//           0, maxCol,      0, 1,
//      maxCol, maxCol,      0, 1,
//      maxCol,      0,      0, 1,
//   };
//
//   mGL->renderColorVertexArray(colorWheelPoints, colorArray, 8, GLOPT::TriangleFan);

   NVGpaint paint;
   F32 centerx = colorWheelPoints[0];
   F32 centery = colorWheelPoints[1];
   F32 rx = colorWheelPoints[2];
   F32 ry = colorWheelPoints[3];
   F32 mx = colorWheelPoints[4];
   F32 my = colorWheelPoints[5];
   F32 bx = colorWheelPoints[6];
   F32 by = colorWheelPoints[7];
   F32 cx = colorWheelPoints[8];
   F32 cy = colorWheelPoints[9];
   F32 gx = colorWheelPoints[10];
   F32 gy = colorWheelPoints[11];
   F32 yx = colorWheelPoints[12];
   F32 yy = colorWheelPoints[13];

   NVGcolor centerCol = nvgRGBAf(maxCol,maxCol,maxCol,1);
   NVGcolor centerColOuter = nvgRGBAf(maxCol,maxCol,maxCol,0);
   NVGcolor redCol = nvgRGBAf(maxCol,0,0,1);
   NVGcolor greenCol = nvgRGBAf(0,maxCol,0,1);
   NVGcolor blueCol = nvgRGBAf(0,0,maxCol,1);
   NVGcolor magentaCol = nvgRGBAf(maxCol,0,maxCol,1);
   NVGcolor cyanCol = nvgRGBAf(0,maxCol,maxCol,1);
   NVGcolor yellowCol = nvgRGBAf(maxCol,maxCol,0,1);

   // START COLOR WHEEL

   // Triangle 1
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, centerx, centery);
   nvgLineTo(nvg, rx, ry);
   nvgLineTo(nvg, mx, my);
   nvgClosePath(nvg);
   // Red to magenta
   paint = nvgRadialGradient(nvg, rx, ry, 0, colorWheel_rad/2,
         redCol, magentaCol);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);
   // Lightness fill
   paint = nvgRadialGradient(nvg, centerx, centery, 0, colorWheel_rad/2,
         centerCol, centerColOuter);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);

   // Triangle 2
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, centerx, centery);
   nvgLineTo(nvg, rx, ry);
   nvgLineTo(nvg, yx, yy);
   nvgClosePath(nvg);
   // Red to yellow
   paint = nvgRadialGradient(nvg, rx, ry, 0, colorWheel_rad/2,
         redCol, yellowCol);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);
   // Lightness fill
   paint = nvgRadialGradient(nvg, centerx, centery, 0, colorWheel_rad/2,
         centerCol, centerColOuter);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);

   // Triangle 3
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, centerx, centery);
   nvgLineTo(nvg, bx, by);
   nvgLineTo(nvg, mx, my);
   nvgClosePath(nvg);
   // Blue to magenta
   paint = nvgRadialGradient(nvg, bx, by, 0, colorWheel_rad/2,
         blueCol, magentaCol);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);
   // Lightness fill
   paint = nvgRadialGradient(nvg, centerx, centery, 0, colorWheel_rad/2,
         centerCol, centerColOuter);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);

   // Triangle 4
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, centerx, centery);
   nvgLineTo(nvg, bx, by);
   nvgLineTo(nvg, cx, cy);
   nvgClosePath(nvg);
   // Blue to cyan
   paint = nvgRadialGradient(nvg, bx, by, 0, colorWheel_rad/2,
         blueCol, cyanCol);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);
   // Lightness fill
   paint = nvgRadialGradient(nvg, centerx, centery, 0, colorWheel_rad/2,
         centerCol, centerColOuter);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);

   // Triangle 5
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, centerx, centery);
   nvgLineTo(nvg, gx, gy);
   nvgLineTo(nvg, cx, cy);
   nvgClosePath(nvg);
   // Green to cyan
   paint = nvgRadialGradient(nvg, gx, gy, 0, colorWheel_rad/2,
         greenCol, cyanCol);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);
   // Lightness fill
   paint = nvgRadialGradient(nvg, centerx, centery, 0, colorWheel_rad/2,
         centerCol, centerColOuter);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);

   // Triangle 6
   nvgBeginPath(nvg);
   nvgMoveTo(nvg, centerx, centery);
   nvgLineTo(nvg, gx, gy);
   nvgLineTo(nvg, yx, yy);
   nvgClosePath(nvg);
   // Green to yellow
   paint = nvgRadialGradient(nvg, gx, gy, 0, colorWheel_rad/2,
         greenCol, yellowCol);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);
   // Lightness fill
   paint = nvgRadialGradient(nvg, centerx, centery, 0, colorWheel_rad/2,
         centerCol, centerColOuter);
   nvgFillPaint(nvg, paint);
   nvgFill(nvg);

   // END COLOR WHEEL

   // Draw the color bars
   // Darkness
   RenderUtils::drawRectVertGradient(colorBrightness_x, colorBrightness_y,
         colorBrightness_w, colorBrightness_h,
         Color(r2,g2,b2), 1.0f, Colors::black, 1.0f);

   // Red/cyan
   RenderUtils::drawRectVertGradient(colorBrightness_x + colorBrightness_w_space, colorBrightness_y,
         colorBrightness_w, colorBrightness_h,
         Color(1.0f,g,b), 1.0f, Color(0.0f,g,b), 1.0f);

   // Green/magenta
   RenderUtils::drawRectVertGradient(colorBrightness_x + 2 * colorBrightness_w_space, colorBrightness_y,
         colorBrightness_w, colorBrightness_h,
         Color(r,1.0f,b), 1.0f, Color(r,0.0f,b), 1.0f);

   // Blue/yellow
   RenderUtils::drawRectVertGradient(colorBrightness_x + 3 * colorBrightness_w_space, colorBrightness_y,
         colorBrightness_w, colorBrightness_h,
         Color(r,g,1.0f), 1.0f, Color(r,g,0.0f), 1.0f);

   // Outlines
   // Wheel
   RenderUtils::drawLineLoop(&colorWheelPoints[2],       6, Colors::white);
   // Bars
   RenderUtils::drawLineLoop(colorBrightnessPoints,      4, Colors::white);
   RenderUtils::drawLineLoop(colorBrightnessPointsRed,   4, Colors::white);
   RenderUtils::drawLineLoop(colorBrightnessPointsGreen, 4, Colors::white);
   RenderUtils::drawLineLoop(colorBrightnessPointsBlue,  4, Colors::white);


   F32 pointerArrow[8];
   pointerArrow[0] = colorBrightness_x + colorBrightness_w; 
   pointerArrow[1] = colorBrightness_y + colorBrightness_h - maxCol * colorBrightness_h;
   drawArrow(pointerArrow, Colors::white);

   pointerArrow[0] = colorBrightness_x + colorBrightness_w + colorBrightness_w_space; 
   pointerArrow[1] = colorBrightness_y + colorBrightness_h - r * colorBrightness_h;
   drawArrow(pointerArrow, Colors::white);

   pointerArrow[0] = colorBrightness_x + colorBrightness_w + colorBrightness_w_space * 2; 
   pointerArrow[1] = colorBrightness_y + colorBrightness_h - g * colorBrightness_h;
   drawArrow(pointerArrow, Colors::white);

   pointerArrow[0] = colorBrightness_x + colorBrightness_w + colorBrightness_w_space * 3; 
   pointerArrow[1] = colorBrightness_y + colorBrightness_h - b * colorBrightness_h;
   drawArrow(pointerArrow, Colors::white);


   if(maxCol != 0)
   {
      F32 x;
      F32 y;

      if(b == maxCol)
      {
         x = (g2 - r2) * .5f + .5f;
         y = 1.f - (g2 + r2) * .25f;
      }
      else if(r == maxCol)
      {
         x = g2 * .5f;
         y = b2 * .5f - g2 * .25f + .25f;
      }
      else
      {
         x = 1.f - r2 * .5f;
         y = b2 * .5f - r2 * .25f + .25f;
      }

      pointerArrow[0] = colorWheel_w * x + colorWheel_x - 10; pointerArrow[1] = colorWheel_h * y + colorWheel_y;
      pointerArrow[2] = pointerArrow[0] + 20;                 pointerArrow[3] = pointerArrow[1];
      pointerArrow[4] = pointerArrow[0] + 10;                 pointerArrow[5] = pointerArrow[1] + 10;
      pointerArrow[6] = pointerArrow[0] + 10;                 pointerArrow[7] = pointerArrow[1] - 10;

      RenderUtils::drawLines(pointerArrow, 4, maxCol > 0.6 ? Colors::black : Colors::white);
   }


   // Render some samples in selected color
   static const F32 x = 50;
   static const F32 y = 540;
   static const F32 h = 50;


   // Loadout zone
   static const Point pointAry[] = { Point(x, y), Point(x + h, y), Point(x + h, y + h), Point(x, y + h) };
   static const Vector<Point> o(pointAry, ARRAYSIZE(pointAry)); 
   Vector<Point> f;     // fill
   Triangulate::Process(o, f);
   GameObjectRender::renderLoadoutZone(*this, &o, &f, Point(x + h/2, y + h/2), 0);

   // Ship
   static F32 thrusts[4] =  { 1, 0, 0, 0 };

   nvgSave(nvg);
   nvgTranslate(nvg, 165, y + h / 2);
   nvgRotate(nvg, -FloatHalfPi);

   GameObjectRender::renderShip(ShipShape::Normal, *this, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false);

   nvgRestore(nvg);

   // Turret
   nvgSave(nvg);
   nvgTranslate(nvg, 240, y + h / 2);

   GameObjectRender::renderTurret(*this, Point(0, 15), Point(0, -1), true, 1, 0, 0);

   nvgRestore(nvg);
}


void UIColorPicker::quit()
{
   // Do nothing
}


bool UIColorPicker::onKeyDown(InputCode inputCode)
{
   if(inputCode == KEY_ESCAPE || inputCode == BUTTON_BACK)       // Quit
   {
      getUIManager()->getPrevUI()->onColorPicked(*this);
      getUIManager()->reactivatePrevUI();
   }
   else if(inputCode == KEY_R)
   {
      r += (InputCodeManager::checkModifier(KEY_SHIFT) ? -.01f : .01f);
      limitRange(*this);
   }
   else if(inputCode == KEY_G)
   {
      g += (InputCodeManager::checkModifier(KEY_SHIFT) ? -.01f : .01f);
      limitRange(*this);
   }
   else if(inputCode == KEY_B)
   {
      b += (InputCodeManager::checkModifier(KEY_SHIFT) ? -.01f : .01f);
      limitRange(*this);
   }
   else if(inputCode == MOUSE_LEFT)
   {
      F32 x = DisplayManager::getScreenInfo()->getMousePos()->x;
      F32 y = DisplayManager::getScreenInfo()->getMousePos()->y;
      if(x >= colorWheel_x && x <= colorWheel_x + colorWheel_w && y >= colorWheel_y && y <= colorWheel_y + colorWheel_h)
         mMouseDown = 1;
      else if(x >= colorBrightness_x && x < colorBrightness_x + colorBrightness_w_space*4 && y >= colorBrightness_y && y <= colorBrightness_y + colorBrightness_h)
         mMouseDown = 2 + U32((x - colorBrightness_x) / colorBrightness_w_space);
      else if(y >= 525 && x > 300 && x < 500)
      {
         getUIManager()->getPrevUI()->onColorPicked(*this);
         getUIManager()->reactivatePrevUI();
      }
      else if(y >= 575 && x > 700)
         getUIManager()->reactivatePrevUI();

      onMouseMoved();
   }

   else
      return Parent::onKeyDown(inputCode);

   return true;
}


void UIColorPicker::onKeyUp(InputCode inputCode)
{
   if(inputCode == MOUSE_LEFT)
      mMouseDown = false;
   else
      Parent::onKeyUp(inputCode);
}
void UIColorPicker::onMouseMoved()
{
   F32 x = DisplayManager::getScreenInfo()->getMousePos()->x;
   F32 y = DisplayManager::getScreenInfo()->getMousePos()->y;

   F32 maxCol = r > g ? r : g;
   if(b > maxCol)
      maxCol = b;

   if(mMouseDown == 1 && maxCol != 0)
   {
      x = (x - colorWheel_x) / colorWheel_w;
      y = (y - colorWheel_y) / colorWheel_h;
      if(x < 0) x = 0;
      if(x > 1) x = 1;
      if(y < 0) y = 0;
      if(y > 1) y = 1;

      if(x < .5f && y + x * .5f < .75f)
      {
         r = maxCol;
         g = (x * 2) * maxCol;
         b = (y * 2 - .5f + x) * maxCol;
      }
      else if(y - x * .5f < .25f)
      {
         r = (2 - x * 2) * maxCol;
         g = maxCol;
         b = (y * 2 + .5f - x) * maxCol;
      }
      else
      {
         r = (-x - y * 2 + 2.5f) * maxCol;
         g = (x - y * 2 + 1.5f) * maxCol;
         b = maxCol;
      }
   }

   else if(mMouseDown >= 2 && mMouseDown <= 5)
   {
      y = 1 - (y - colorBrightness_y) / colorBrightness_h;
      if(y < 0) y = 0;
      if(y > 1) y = 1;

      switch(mMouseDown)
      {
      case 2:
         if(maxCol == 0)
         {
            b = g = r = y;
         }
         else
         {
            F32 adjust = y / maxCol;
            r *= adjust;
            g *= adjust;
            b *= adjust;
         }
         break;
      case 3:
         r = y;
         break;
      case 4:
         g = y;
         break;
      case 5:
         b = y;
      }
   }

   limitRange(*this);
}


}
