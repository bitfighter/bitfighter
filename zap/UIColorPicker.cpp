//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------
#include "UIColorPicker.h"

#include "gameObjectRender.h"
#include "UIManager.h"
#include "FontManager.h"
#include "Cursor.h"
#include "Colors.h"
#include "ship.h"

#include "OpenglUtils.h"
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

static void drawArrow(F32 *p)
{
   p[2] = p[0] + 20; p[3] = p[1] - 10;
   p[4] = p[0] + 20; p[5] = p[1] + 10;
   renderVertexArray(p, 3, GL_LINE_LOOP);
}

const S32 colorWheel_x = 100;
const S32 colorWheel_y = 100;
const S32 colorWheel_w = 400;
const S32 colorWheel_h = 400;

const S32 colorBrightness_x = 550;
const S32 colorBrightness_y = 100;
const S32 colorBrightness_w = 25;
const S32 colorBrightness_w_space = 50;
const S32 colorBrightness_h = 400;

UIColorPicker::UIColorPicker(ClientGame *game) : Parent(game) {mMouseDown = 0;}
UIColorPicker::~UIColorPicker(){ /* Do nothing */ }


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
   colorWheel_w / 2 + colorWheel_x, colorWheel_h / 2     + colorWheel_y,
                      colorWheel_x, colorWheel_h / 4     + colorWheel_y,
                      colorWheel_x, colorWheel_h * 3 / 4 + colorWheel_y,
   colorWheel_w / 2 + colorWheel_x, colorWheel_h         + colorWheel_y,
   colorWheel_w     + colorWheel_x, colorWheel_h * 3 / 4 + colorWheel_y,
   colorWheel_w     + colorWheel_x, colorWheel_h / 4     + colorWheel_y,
   colorWheel_w / 2 + colorWheel_x,                        colorWheel_y,
                      colorWheel_x, colorWheel_h / 4     + colorWheel_y,
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


void UIColorPicker::render() const
{
   glColor(Colors::green);

   FontManager::pushFontContext(MenuHeaderContext);
   drawCenteredUnderlinedString(15, 30, "COLOR PICKER");
   drawStringc(400, 580, 30, "Done");
   drawString (730, 580, 15, "Cancel");
   FontManager::popFontContext();

   glColor(Colors::white);

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

   F32 colorArray[32] = {
      maxCol, maxCol ,maxCol, 1,
      maxCol,      0,      0, 1,
      maxCol,      0, maxCol, 1,
           0,      0, maxCol, 1,
           0, maxCol, maxCol, 1,
           0, maxCol,      0, 1,
      maxCol, maxCol,      0, 1,
      maxCol,      0,      0, 1,
   };

   renderColorVertexArray(colorWheelPoints, colorArray, 8, GL_TRIANGLE_FAN);


   colorArray[0]  = r2;  colorArray[1]  = g2;  colorArray[2]  = b2;
   colorArray[4]  = r2;  colorArray[5]  = g2;  colorArray[6]  = b2; 
   colorArray[8]  = 0;   colorArray[9]  = 0;   colorArray[10] = 0; 
   colorArray[12] = 0;   colorArray[13] = 0;   colorArray[14] = 0;
   renderColorVertexArray(colorBrightnessPoints, colorArray, 4, GL_TRIANGLE_FAN);

   colorArray[0]  = 1;  colorArray[1]  = g;  colorArray[2]  = b;
   colorArray[4]  = 1;  colorArray[5]  = g;  colorArray[6]  = b;
   colorArray[9]  = g;  colorArray[10] = b;
   colorArray[13] = g;  colorArray[14] = b;
   renderColorVertexArray(colorBrightnessPointsRed, colorArray, 4, GL_TRIANGLE_FAN);

   colorArray[0]  = r; colorArray[1]  = 1;
   colorArray[4]  = r; colorArray[5]  = 1;
   colorArray[8]  = r; colorArray[9]  = 0;
   colorArray[12] = r; colorArray[13] = 0;
   renderColorVertexArray(colorBrightnessPointsGreen, colorArray, 4, GL_TRIANGLE_FAN);

   colorArray[1]  = g; colorArray[2]  = 1;
   colorArray[5]  = g; colorArray[6]  = 1;
   colorArray[9]  = g; colorArray[10] = 0;
   colorArray[13] = g; colorArray[14] = 0;
   renderColorVertexArray(colorBrightnessPointsBlue, colorArray, 4, GL_TRIANGLE_FAN);

   glColor(Colors::white);
   renderVertexArray(&colorWheelPoints[2],       6, GL_LINE_LOOP);
   renderVertexArray(colorBrightnessPoints,      4, GL_LINE_LOOP);
   renderVertexArray(colorBrightnessPointsRed,   4, GL_LINE_LOOP);
   renderVertexArray(colorBrightnessPointsGreen, 4, GL_LINE_LOOP);
   renderVertexArray(colorBrightnessPointsBlue,  4, GL_LINE_LOOP);


   F32 pointerArrow[8];
   pointerArrow[0] = colorBrightness_x + colorBrightness_w; pointerArrow[1] = colorBrightness_y + colorBrightness_h - maxCol * colorBrightness_h;
   drawArrow(pointerArrow);

   pointerArrow[0] = colorBrightness_x + colorBrightness_w + colorBrightness_w_space; pointerArrow[1] = colorBrightness_y + colorBrightness_h - r * colorBrightness_h;
   drawArrow(pointerArrow);

   pointerArrow[0] = colorBrightness_x + colorBrightness_w + colorBrightness_w_space * 2; pointerArrow[1] = colorBrightness_y + colorBrightness_h - g * colorBrightness_h;
   drawArrow(pointerArrow);

   pointerArrow[0] = colorBrightness_x + colorBrightness_w + colorBrightness_w_space * 3; pointerArrow[1] = colorBrightness_y + colorBrightness_h - b * colorBrightness_h;
   drawArrow(pointerArrow);


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

      glColor(maxCol > 0.6 ?  0.0f : 1.0f);

      renderVertexArray(pointerArrow, 4, GL_LINES);
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
   renderLoadoutZone(*this, &o, &f, Point(x + h/2, y + h/2), 0);

   // Ship
   static F32 thrusts[4] =  { 1, 0, 0, 0 };

   glPushMatrix();
   glTranslate(165, y + h / 2);
   glRotate(-90);

   renderShip(ShipShape::Normal, *this, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false);

   glPopMatrix();

   // Turret
   glPushMatrix();
   glTranslate(240, y + h / 2);

   renderTurret(*this, Point(0, 15), Point(0, -1), true, 1, 0, 0);

   glPopMatrix();
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
