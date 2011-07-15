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

#include "SDL/SDL_opengl.h"

using namespace TNL;

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
      glColor3f(1,0,0);
   else
      glColor3f(1,1,1);
}

// Render some specific, common button types
void JoystickRender::renderRoundButton(Point loc, const char *label, AlignType align, bool activated)
{
   const S32 radius = roundButtonRadius;
   const S32 labelSize = 12;

   Point offset;

   if(align == ALIGN_LEFT)
   {
      offset = Point(radius, radius);
   }
   else if (align == ALIGN_RIGHT)
   {
      offset = Point(-radius, radius);
   }
   else  // ALIGN_CENTER
   {
      offset = Point(0, radius);
   }

   setButtonColor(activated);

   drawCircle(loc + offset, radius);
   UserInterface::drawString(loc.x + offset.x - UserInterface::getStringWidth(labelSize, label) / 2, loc.y + 2, labelSize, label);
}


void JoystickRender::renderRectButton(Point loc, const char *label, AlignType align, bool activated)
{
   const S32 width = rectButtonWidth;
   const S32 halfWidth = 12;
   const S32 height = rectButtonHeight;
   const S32 halfHeight = 9;
   const S32 labelSize = 12;

   Point offset;

   if(align == ALIGN_LEFT)
   {
      offset = Point(halfWidth, halfHeight);
   }
   else if (align == ALIGN_RIGHT)
   {
      offset = Point(-halfWidth, halfHeight);
   }
   else  // ALIGN_CENTER
   {
      offset = Point(0,halfHeight);
   }

   setButtonColor(activated);

   drawRoundedRect(loc + offset, width, height, 3);

   UserInterface::drawString(loc.x + offset.x - UserInterface::getStringWidth(labelSize, label) / 2, loc.y + 2, labelSize, label);
}


void JoystickRender::renderSmallRectButton(Point loc, const char *label, AlignType align, bool activated)
{
   const S32 width = smallRectButtonWidth;
   const S32 halfWidth = 10;
   const S32 height = 15;
   const S32 halfHeight = 8;
   const S32 labelSize = 12;

   Point offset;

   if(align == ALIGN_LEFT)
   {
      offset = Point(halfWidth, halfHeight);
   }
   else if (align == ALIGN_RIGHT)
   {
      offset = Point(-halfWidth, halfHeight);
   }
   else  // ALIGN_CENTER
   {
      offset = Point(0,halfHeight);
   }

   setButtonColor(activated);

   drawRoundedRect(loc + offset, width, height, 3);

   UserInterface::drawString(loc.x + offset.x - UserInterface::getStringWidth(labelSize, label) / 2, loc.y + 2, labelSize, label);
}


// Render dpad graphic
void JoystickRender::renderDPad(Point center, F32 radius, bool upActivated, bool downActivated, bool leftActivated,
                bool rightActivated, const char *msg1, const char *msg2)
{
   radius = radius * 0.143;   // = 1/7  Correct for the fact that when radius = 1, graphic has 7 px radius

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


extern IniSettings gIniSettings;
extern S32 keyCodeToButtonIndex(KeyCode keyCode);

// Only partially implemented at the moment...
S32 JoystickRender::getControllerButtonRenderedSize(KeyCode keyCode)
{
   S32 joy = gIniSettings.joystickType;
   //InputMode inputMode = gIniSettings.inputMode;

   if(!isControllerButton(keyCode))    // Render keyboard keys
      return UserInterface::getStringWidthf(15, "[%s]", keyCodeToString(keyCode));

   // Render controller button
   U32 buttonIndex = keyCodeToButtonIndex(keyCode);

   if(joy == LogitechWingman)
   {
      if (buttonIndex < 6)
         return roundButtonRadius;
      else if(buttonIndex < 8)
         return -1;     // No buttons 7 & 8
      else return rectButtonWidth;
   }

   else if(joy == LogitechDualAction)
   {
      if(buttonIndex < 4)   // 4 round buttons on top
         return roundButtonRadius;
      else if(buttonIndex < 8)   // 4 shoulder buttons
         return rectButtonWidth;
      else                       // 2 buttons on top
         return smallRectButtonWidth;
   }

   else if(joy == SaitekDualAnalogP880)    // 8 round buttons, no start, one large red button
   {
      if(buttonIndex < 6)            // 6 round buttons on top
         return roundButtonRadius;
      else if(buttonIndex < 8)       // 2 shoulder triggers
         return rectButtonWidth;
      else if(buttonIndex < 9)      // start  -> S button?
         return 12;
      else                          // No back button?
        return -1;
   }

   else if(joy == SaitekDualAnalogRumblePad)    // First 4 buttons are circles, next 4 are rectangles
   {
      if(buttonIndex < 4)           // 4 round buttons on top
         return roundButtonRadius;
      else if(buttonIndex < 8)      // 4 shoulder triggers
         return rectButtonWidth;
      else                          // 2 small square buttons on top
         return smallRectButtonWidth;
   }

   // http://ecx.images-amazon.com/images/I/412Q3RFHZVL._SS500_.jpg
   else if(joy == PS2DualShock || joy == PS2DualShockConversionCable)
   {
      if(buttonIndex < 4)
         return 18;
      else
         return rectButtonWidth;
   }
  // http://ps3media.ign.com/ps3/image/article/705/705934/e3-2006-in-depth-with-the-ps3-controller-20060515010609802.jpg
   else if(joy == PS3DualShock)
   {
      if(buttonIndex < 4)
         return 18;
      else
         return rectButtonWidth;
   }
   else if(joy == XBoxController || joy == XBoxControllerOnXBox)
   {
      if(buttonIndex < 4)
         return 18;
      else
         return rectButtonWidth;
   }
   else  // Something generic
   {
      return roundButtonRadius;
   }

   return -1;     // Kill a useless warning
}


// Renders something resembling a controller button or keyboard key
void JoystickRender::renderControllerButton(F32 x, F32 y, KeyCode keyCode, bool activated, S32 offset)
{
   S32 joy = gIniSettings.joystickType;
   //InputMode inputMode = gIniSettings.inputMode;

   if(!isControllerButton(keyCode))    // Render keyboard keys
   {
      UserInterface::drawStringf(x, y, 15, "[%s]", keyCodeToString(keyCode));
      return;
   }

   // Render controller button

   x += (F32) offset;
   U32 buttonIndex = keyCodeToButtonIndex(keyCode);     // Index ranges from 0 to 9 for the 10 buttons

   // http://www.amazon.com/Logitech-963196-0403-WingMan-GamePad/dp/B00004RCG1/ref=pd_bbs_sr_5
   // 6 round buttons on top (ABCXYZ) plus start/select button, plus two triggers

   if(joy == LogitechWingman)
   {
      string labels[10] = { "A", "B", "C", "X", "Y", "Z", "", "", "T1", "T2" };
      if (buttonIndex < 6)
         renderRoundButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
      else if(buttonIndex < 8)
         return;     // These buttons don't exist
      else
         renderRectButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
   }

   // http://www.amazon.com/Logitech-Dual-Action-Game-Pad/dp/B0000ALFCI/ref=pd_bbs_sr_1?ie=UTF8&s=electronics&qid=1200603250&sr=8-1
   // 4 round buttons on top (1-4), 4 shoulder buttons (5-8), 2 small square buttons on top (9, 10?)
   else if(joy == LogitechDualAction)
   {
      string labels[10] = { "1", "2", "3", "4", "7", "8", "5", "6", "9", "10" };
      if(buttonIndex < 4)        // 4 round buttons on top
         renderRoundButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
      else if(buttonIndex < 8)   // 4 shoulder buttons
         renderRectButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
      else                       // 2 buttons on top
         renderSmallRectButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);

   }

   // http://www.madtech.pl/pliki/saitek/p880/1.jpg
   else if(joy == SaitekDualAnalogP880)    // 8 round buttons, one red one, no start button
   {
      string labels[] = { "1", "2", "3", "4", "5", "6", "7", "8" };
      setButtonColor(activated);
      if(buttonIndex < 6)        // 6 round buttons on top
         renderRoundButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);

      else if(buttonIndex < 8)   // 2 shoulder triggers
         renderRectButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
      else if(buttonIndex < 9)   // S button on top
      {
         // Draw button centered on x,y
         glColor3f(1, 0, 0);
         drawFilledEllipse(Point(x, y + 8), 12, 6, 0);

         setButtonColor(activated);
         drawEllipse(Point(x, y + 8), 12, 6, 0);

         glColor3f(1, 1, 1);
         UserInterface::drawString(x - 4, y + 1, 11, "S");
     }
      else
         return;                 // No back button
   }

   else if(joy == SaitekDualAnalogRumblePad)    // First 4 buttons are circles, next 4 are rectangles   (SAITEK P-480 DUAL-ANALOG)
   {
      string labels[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "10", "9" };    // Yes, I mean 10, 9!
      setButtonColor(activated);
      if(buttonIndex < 4)           // 4 round buttons on top
         renderRoundButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);

      else if(buttonIndex < 8)      // 4 shoulder triggers
         renderRectButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);

      else                          // 2 small square buttons on top
         renderSmallRectButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
   }

   // http://ecx.images-amazon.com/images/I/412Q3RFHZVL._SS500_.jpg
   else if(joy == PS2DualShock || joy == PS2DualShockConversionCable)
   {
      if(buttonIndex >= Joystick::PredefinedJoystickList[joy].buttonCount)
         return;

      static F32 color[6][3] = {
         { 0.5, 0.5, 1 },
         { 1, 0.5, 0.5 },
         { 1, 0.5, 1 },
         { 0.5, 1, 0.5 },
         { 0.7, 0.7, 0.7 },
         { 0.7, 0.7, 0.7 }
      };
      Color c(color[buttonIndex][0], color[buttonIndex][1], color[buttonIndex][2]);
      Point center(x, y + 8);
      if(buttonIndex < 4)
      {
         setButtonColor(activated);
         drawCircle(center, 9);
         glColor(c);
         switch(buttonIndex)
         {
         case 0:     // X
            glBegin(GL_LINES);
            glVertex(center + Point(-5, -5));
            glVertex(center + Point(5, 5));
            glVertex(center + Point(-5, 5));
            glVertex(center + Point(5, -5));
            glEnd();
            break;
         case 1:     // Circle
            drawCircle(center, 6);
            break;
         case 2:     // Square
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(-5, -5));
            glVertex(center + Point(-5, 5));
            glVertex(center + Point(5, 5));
            glVertex(center + Point(5, -5));
            glEnd();
            break;
         case 3:     // Triangle
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(0, -7));
            glVertex(center + Point(6, 5));
            glVertex(center + Point(-6, 5));
            glEnd();
            break;
         }
      }
      else
      {
         string labels[] = { "L2", "R2", "L1", "R1", "Sel", "Strt" };
         if (buttonIndex < 9)    // Shoulder buttons
            renderRectButton(Point(x, y), labels[buttonIndex - 4].c_str(), ALIGN_CENTER, activated);
         else
         {  // Triangle button
            setButtonColor(activated);
            glBegin(GL_LINE_LOOP);
               glVertex(center + Point(-15, -9));
               glVertex(center + Point(-15, 10));
               glVertex(center + Point(12, 0));
            glEnd();
           UserInterface::drawString(x - 13, y + 3, 8, labels[buttonIndex - 4].c_str());
         }
      }
   }
   // http://ps3media.ign.com/ps3/image/article/705/705934/e3-2006-in-depth-with-the-ps3-controller-20060515010609802.jpg
   // Based on PS2DualShock
   else if(joy == PS3DualShock)
   {
      if(buttonIndex >= Joystick::PredefinedJoystickList[joy].buttonCount)
         return;

      static F32 color[6][3] = {
            { 0.5, 0.5, 1 },
            { 1, 0.5, 0.5 },
            { 1, 0.5, 1 },
            { 0.5, 1, 0.5 },
            { 0.7, 0.7, 0.7 },
            { 0.7, 0.7, 0.7 }
      };
      Color c(color[buttonIndex][0], color[buttonIndex][1], color[buttonIndex][2]);
      Point center(x, y + 8);
      if(buttonIndex < 4)
      {
         setButtonColor(activated);
         drawCircle(center, 9);
         glColor(c);
         switch(buttonIndex)
         {
         case 0:     // X
            glBegin(GL_LINES);
            glVertex(center + Point(-5, -5));
            glVertex(center + Point(5, 5));
            glVertex(center + Point(-5, 5));
            glVertex(center + Point(5, -5));
            glEnd();
            break;
         case 1:     // Circle
            drawCircle(center, 6);
            break;
         case 2:     // Square
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(-5, -5));
            glVertex(center + Point(-5, 5));
            glVertex(center + Point(5, 5));
            glVertex(center + Point(5, -5));
            glEnd();
            break;
         case 3:     // Triangle
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(0, -7));
            glVertex(center + Point(6, 5));
            glVertex(center + Point(-6, 5));
            glEnd();
            break;
         }
      }
      else
      {
         string labels[] = { "L2", "R2", "L1", "R1", "Sel", "Strt" };
         if (buttonIndex < 9)    // Shoulder buttons
            renderRectButton(Point(x, y), labels[buttonIndex - 4].c_str(), ALIGN_CENTER, activated);
         else
         {  // Triangle button
            setButtonColor(activated);
            glBegin(GL_LINE_LOOP);
            glVertex(center + Point(-15, -9));
            glVertex(center + Point(-15, 10));
            glVertex(center + Point(12, 0));
            glEnd();
            UserInterface::drawString(x - 13, y + 3, 8, labels[buttonIndex - 4].c_str());
         }
      }
   }
   else if(joy == XBoxController || joy == XBoxControllerOnXBox)
   {
      if(buttonIndex >= Joystick::PredefinedJoystickList[joy].buttonCount)
         return;

      static F32 color[4][3] = { { 0, 1, 0 },
                                 { 1, 0, 0 },
                                 { 0, 0, 1 },
                                 { 1, 1, 0 } };

      if(buttonIndex <= 3)
      {
         Color c(color[buttonIndex][0], color[buttonIndex][1], color[buttonIndex][2]);
         glColor(c * 0.4f);
         drawFilledCircle(Point(x, y + 8), 9);
         setButtonColor(activated);
         drawCircle(Point(x, y + 8), 9);

         const char buttons[] = "ABXY";
         glColor(c);
         UserInterface::drawStringf(x - 4, y + 2, 12, "%c", buttons[buttonIndex]);
      }

      if(buttonIndex == 4 || buttonIndex == 5)     // RB, LB
      {
         setButtonColor(activated);
         drawRoundedRect(Point(x, y + 8), rectButtonWidth, rectButtonHeight, 3);
         glColor3f(1,1,1);
         UserInterface::drawString(x - 7, y + 1, 12, buttonIndex == 5 ? "LB" : "RB");
      }

      if(buttonIndex == 6 || buttonIndex == 7)     // RT, LT
      {
         setButtonColor(activated);
         drawRoundedRect(Point(x, y + 8), rectButtonWidth, rectButtonHeight, 3);
         glColor3f(1,1,1);
         UserInterface::drawString(x - 7, y + 1, 12, buttonIndex == 7 ? "LT" : "RT");
      }


      else if(buttonIndex == 8 || buttonIndex == 9)      // Render right/left-pointing triangle in an ovally-square button
      {
         setButtonColor(activated);
         drawRoundedRect(Point(x, y + 8), 20, 15, 5);
         glColor3f(1,1,1);
         S32 dir = (buttonIndex == 9) ? -1 : 1;
         glBegin(GL_LINE_LOOP);
            glVertex(Point(x + dir * 4, y + 8));
            glVertex(Point(x - dir * 3, y + 13));
            glVertex(Point(x - dir * 3, y + 3));
         glEnd();
      }
   }

   else  // Something generic
   {
      string labels[10] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0" };
      renderRoundButton(Point(x, y), labels[buttonIndex].c_str(), ALIGN_CENTER, activated);
   }
}

////////// End rendering functions

} /* namespace Zap */
