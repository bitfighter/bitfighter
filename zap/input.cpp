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

#include "input.h"
#include "move.h"
#include "UIMenus.h"
#include "Point.h"

#include "tnlJournal.h"

#include "gameObjectRender.h"
#include "config.h"
#include "Event.h"

#include "SDL/SDL_opengl.h"

#include <math.h>
#include <string>

namespace Zap
{
JoystickMapping gJoystickMapping;


// First, some variables to hold our various non-keyboard inputs
enum JoystickAxes {
   AXIS_LEFT_RIGHT,
   AXIS_UP_DOWN,
   AXIS_COUNT
};

enum JoysticksEnum {
   JOYSTICK_MOVE,
   JOYSTICK_FIRE,
   JOYSTICK_COUNT
};

enum AlignType {
   ALIGN_LEFT,
   ALIGN_CENTER,
   ALIGN_RIGHT
};

F32 gJoystickInput[JOYSTICK_COUNT][AXIS_COUNT];
U32 gRawJoystickButtonInputs;
F32 gRawAxisButtonInputs[MaxJoystickAxes];


extern void drawCircle(const Point &pos, F32 radius);

inline void setButtonColor(bool activated)
{
   if(activated)
      glColor3f(1,0,0);
   else
      glColor3f(1,1,1);
}

static S32 keyCodeToButtonIndex(KeyCode keyCode)
{
   switch(keyCode)
   {
      case BUTTON_1:
         return 0;
      case BUTTON_2:
         return 1;
      case BUTTON_3:
         return 2;
      case BUTTON_4:
         return 3;
      case BUTTON_5:
         return 4;
      case BUTTON_6:
         return 5;
      case BUTTON_7:
         return 6;
      case BUTTON_8:
         return 7;
      case BUTTON_START:
         return 8;
      case BUTTON_BACK:
         return 9;
      case BUTTON_DPAD_UP:
         return 10;
      case BUTTON_DPAD_DOWN:
         return 11;
      case BUTTON_DPAD_LEFT:
         return 12;
      case BUTTON_DPAD_RIGHT:
         return 13;
      default:
         return 9999;
   }
}

// Unfortunately, Linux and Windows map joystick axes differently
// From ControllerTypeType enum ---> LogitechWingman, LogitechDualAction, SaitekDualAnalogP880, SaitekDualAnalogRumblePad, PS2DualShock, PS2DualShockConversionCable, PS3DualShock, XBoxController, XBoxControllerOnXBox                                                                                                                                              \/ FIXME: Someone check PS3 Joypad on windows!
//                                                                Wingmn    DualAct   P880     RumbPad     PS2  PS2 w/Cnvrtr  PS3   XBox    XBoxOnXBox
static U32 controllerButtonCounts[ControllerTypeCount] =          { 9,       10,       9,        10,       10,       10,      10,     10,     14 };     // How many buttons?
#ifdef TNL_OS_LINUX
//                                      ? = need to change?         ?       works       ?          ?        ?         ?        ?         ?         ?
static U32 shootAxisRemaps[ControllerTypeCount][AXIS_COUNT] = { { 5, 6 }, { 2, 3 }, { 5, 2 }, { 5, 2 }, { 2, 5 }, { 5, 2}, { 2, 3 }, { 3, 4 }, { 3, 4 } };  // What axes to use for firing?  Should we let users set this somehow?
#else
static U32 shootAxisRemaps[ControllerTypeCount][AXIS_COUNT] = { { 5, 6 }, { 2, 5 }, { 5, 2 }, { 5, 2 }, { 2, 5 }, { 5, 2}, { 2, 5 }, { 3, 4 }, { 3, 4 } };
#endif

// PS3 Joystick: http://ps3media.ign.com/ps3/image/article/705/705934/e3-2006-in-depth-with-the-ps3-controller-20060515010609802.jpg

static U32 controllerButtonRemaps[ControllerTypeCount][MaxJoystickButtons] =
{
   { // LogitechWingman   9
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton5,
      ControllerButton6,
      ControllerButton7,         // L-Trigger
      ControllerButton8,         // R-Trigger
      ControllerButtonBack,
      0,
      0,
      0,
      0,
      0,
   },
   { // LogitechDualAction   10
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton7,
      ControllerButton8,
      ControllerButton5,
      ControllerButton6,
      ControllerButtonBack,
      ControllerButtonStart,
      0,
      0,
      0,
      0,
   },
   { // SaitekDualAnalogP880  9
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton5,
      ControllerButton6,
      ControllerButton7,
      ControllerButton8,
      0,
      0,
      ControllerButtonBack,         // Red button??...  no start button??
      0,
      0,
      0,
   },
   { // SaitekDualAnalogRumblePad   10       // SAITEK P-480 DUAL-ANALOG
      ControllerButton1,
      ControllerButton2,
      ControllerButton3,
      ControllerButton4,
      ControllerButton5,
      ControllerButton6,
      ControllerButton7,
      ControllerButton8,
      ControllerButtonBack,      // Button 9
      ControllerButtonStart,     // Button 10
      0,
      0,
      0,
      0,
   },
   { // PS2DualShock    10
      ControllerButton4,
      ControllerButton2,
      ControllerButton1,
      ControllerButton3,
      ControllerButton5,
      ControllerButton6,
      ControllerButton7,
      ControllerButton8,
      ControllerButtonBack,
      0,
      0,
      ControllerButtonStart,
      0,
      0,
   },
   { // PS2DualShockConversionCable    10
      ControllerButton4,
      ControllerButton2,
      ControllerButton1,
      ControllerButton3,
      ControllerButton5,
      ControllerButton6,
      ControllerButton7,
      ControllerButton8,
      ControllerButtonBack,
      ControllerButtonStart,
      0,
      0,
      0,
      0,
   },
   { // PS3DualShock    13
      ControllerButtonStart, // Start
      0, // L3 - Unused
      0, // R3 - Unused
      ControllerButtonBack, // Select
      ControllerButtonDPadUp, // DPAD Up
      ControllerButtonDPadRight, // DPAD Right
      ControllerButtonDPadDown, // DPAD Down
      ControllerButtonDPadLeft, // DPAD Left
      ControllerButton5, // L2
      ControllerButton6, // R2
      ControllerButton7, // L1
      ControllerButton8, // R1
      ControllerButton4, // Triangle
      ControllerButton2, // Circle
      // FIXME: If you can get X and Square Working. Add them as these buttons:
      // ControllerButton1 // X
      // ControllerButton3 // Square
      // Above order should be correct. X is Button 14 Square is Button 15.
   },
   { // XBoxController     10
      ControllerButton1,      // A
      ControllerButton2,      // B
      ControllerButton3,      // X
      ControllerButton4,      // Y
      ControllerButton6,      // RB
      ControllerButton5,      // LB
      ControllerButtonBack,   // <
      ControllerButtonStart,  // >
      0,
      0,
      ControllerButton7,
      ControllerButton8,
      0,
      0,
   },
   { // XBoxControllerOnXBox     <--- what's going on here?  On XBox??
      1 << 0,
      1 << 1,
      1 << 2,
      1 << 3,
      1 << 4,
      1 << 5,
      1 << 6,
      1 << 7,
      1 << 8,
      1 << 9,
      1 << 10,
      1 << 11,
      1 << 12,
      1 << 13,
   }
};



///////// Rendering functions

S32 roundButtonRadius = 9;
S32 rectButtonWidth = 24;
S32 rectButtonHeight = 17;
S32 smallRectButtonWidth = 19;

// Render some specific, common button types
void renderRoundButton(Point loc, const char *label, AlignType align, bool activated)
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


void renderRectButton(Point loc, const char *label, AlignType align, bool activated)
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


void renderSmallRectButton(Point loc, const char *label, AlignType align, bool activated)
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
void renderDPad(Point center, F32 radius, bool upActivated, bool downActivated, bool leftActivated,
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

// Only partially implemented at the moment...
S32 getControllerButtonRenderedSize(KeyCode keyCode)
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
void renderControllerButton(F32 x, F32 y, KeyCode keyCode, bool activated, S32 offset)
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
      if(buttonIndex >= controllerButtonCounts[joy])
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
      if(buttonIndex >= controllerButtonCounts[joy])
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
      if(buttonIndex >= controllerButtonCounts[joy])
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

extern Vector<string> gJoystickNames;
extern U32 gUseStickNumber;

ControllerTypeType autodetectJoystickType()
{
   S32 ret = UnknownController;

   TNL_JOURNAL_READ_BLOCK(JoystickAutodetect,
      TNL_JOURNAL_READ((&ret));
      return (ControllerTypeType) ret;
      )

   if(gUseStickNumber > 0)
   {
      if(gJoystickNames[gUseStickNumber - 1] == "WingMan")
         ret = LogitechWingman;

      else if(gJoystickNames[gUseStickNumber - 1] == "XBoxOnXBox")
         ret = XBoxControllerOnXBox;

      // Note that on the only XBox controller I've used on windows, the autodetect string was simply:
      // "Controller (XBOX 360 For Windows)".  I don't know if there are other variations out there.
      else if(gJoystickNames[gUseStickNumber - 1].find("XBOX") != string::npos || 
              gJoystickNames[gUseStickNumber - 1].find("XBox") != string::npos)
         ret = XBoxController;

      else if(gJoystickNames[gUseStickNumber - 1] == "4 axis 16 button joystick")
         ret = PS2DualShock;                                         // http://ecx.images-amazon.com/images/I/412Q3RFHZVL._SS500_.jpg

      else if(gJoystickNames[gUseStickNumber - 1] == "PC Conversion Cable")
         ret = PS2DualShockConversionCable;                          // http://ecx.images-amazon.com/images/I/412Q3RFHZVL._SS500_.jpg

      // Note that on Linux, when I plug in the controller, DMSG outputs Sony PLAYSTATION(R)3 Controller.
      // Was connected.. Thus i have used it here. This may be different on Windows...
      else if(gJoystickNames[gUseStickNumber - 1].find("PLAYSTATION(R)3") != string::npos)
         ret = PS3DualShock;                                                     // http://ps3media.ign.com/ps3/image/article/705/705934/e3-2006-in-depth-with-the-ps3-controller-20060515010609802.jpg

      else if(gJoystickNames[gUseStickNumber - 1] == "P880")
         ret = SaitekDualAnalogP880;

      else if(gJoystickNames[gUseStickNumber - 1] == "Dual Analog Rumble Pad")
         ret = SaitekDualAnalogRumblePad;

      else if(gJoystickNames[gUseStickNumber - 1].find("Logitech Dual Action") != string::npos)
         ret = LogitechDualAction;

      else if(gJoystickNames[gUseStickNumber - 1].find("USB Joystick") != string::npos)
         ret = GenericController;

      else if(gJoystickNames[gUseStickNumber - 1] == "")      // Anything else -- joystick present but unknown
         ret = UnknownController;

      else     // Not sure this can ever happen
         ret = NoController;
   }

   TNL_JOURNAL_WRITE_BLOCK(JoystickAutodetect,
      TNL_JOURNAL_WRITE((ret));
   )
   return (ControllerTypeType) ret;
}


ControllerTypeType stringToJoystickType(string strJoystick)
{
   if (strJoystick == "LogitechWingman")
      return LogitechWingman;
   else if (strJoystick == "LogitechDualAction")
      return LogitechDualAction;
   else if (strJoystick == "SaitekDualAnalogP880")
      return SaitekDualAnalogP880;
   else if (strJoystick == "SaitekDualAnalogRumblePad")
      return SaitekDualAnalogRumblePad;
   else if (strJoystick == "PS2DualShock")
      return PS2DualShock;
   else if (strJoystick == "PS2DualShockConversionCable")
      return PS2DualShockConversionCable;
   else if (strJoystick == "PS3DualShock")
      return PS3DualShock;
   else if (strJoystick == "XBoxController")
      return XBoxController;
   else if (strJoystick == "XBoxControllerOnXBox")
      return XBoxControllerOnXBox;
   else if (strJoystick == "GenericController")
      return GenericController;
   else if (strJoystick == "UnknownController")
      return UnknownController;
   else
      return NoController;
}

string joystickTypeToString(S32 controllerType)
{
   switch (controllerType)
   {
      case LogitechWingman:
         return "LogitechWingman";
      case LogitechDualAction:
         return "LogitechDualAction";
      case SaitekDualAnalogP880:
         return "SaitekDualAnalogP880";
      case SaitekDualAnalogRumblePad:
         return "SaitekDualAnalogRumblePad";
      case PS2DualShock:
         return "PS2DualShock";
      case PS2DualShockConversionCable:
         return "PS2DualShockConversionCable";
      case PS3DualShock:
         return "PS3DualShock";
      case XBoxController:
         return "XBoxController";
      case XBoxControllerOnXBox:
         return "XBoxControllerOnXBox";
      case GenericController:
         return "GenericController";
      case UnknownController:
         return "UnknownController";
      default:
         return "NoController";
   }
}

string joystickTypeToPrettyString(S32 controllerType)
{
   switch (controllerType)
   {
      case LogitechWingman:
         return "Logitech Wingman Dual-Analog";
      case LogitechDualAction:
         return "Logitech Dual Action";
      case SaitekDualAnalogP880:
         return "Saitek P-880 Dual-Analog";
      case SaitekDualAnalogRumblePad:
         return "Saitek P-480 Dual-Analog";
      case PS2DualShock:
         return "PS2 Dualshock USB";
      case PS2DualShockConversionCable:
         return "PS2 Dualshock USB with Conversion Cable";
      case PS3DualShock:
         return "PS3 Sixaxis";
      case XBoxController:
         return "XBox Controller USB";
      case XBoxControllerOnXBox:
         return "XBox Controller";
      case GenericController:
         return "Generic Controller";
      case UnknownController:
         return "Unknown";
      default:
         return "No Controller";
   }
}





bool extern getKeyState(KeyCode keyCode);

// Populates theMove with input from joystick, and also creates some
// simulated keyboard events for menu navigation.
// Runs through this every game tick, regardless of whether or not joystick button was pressed.
// There is kind of a mishmash of stuff thrown in here...
U32 JoystickButtonMask2;
static bool processJoystickInputs( U32 &buttonMask )
{
   // It is unknown how to respond to Unknown joystick types!!
   if(gIniSettings.joystickType >= ControllerTypeCount && !gJoystickMapping.enable)
      return false;

   F32 axes[MaxJoystickAxes];
   static F32 minValues[2] = { - 0.5, -0.5 };
   static F32 maxValues[2] = { 0.5, 0.5 };

   U32 hatMask = 0;     // Holds DPad input status

   if(!ReadJoystick(axes, buttonMask, hatMask))
      return false;     // false = no joystick input

   gRawJoystickButtonInputs = buttonMask;
   for(U32 i=0; i<MaxJoystickAxes; i++)
   {
      gRawAxisButtonInputs[i] = axes[i];
   }

   // All axes return -1 to 1
   // Let's map the controls
   F32 controls[4];     // 0, 1 --> move; 2, 3 --> fire

   if(gJoystickMapping.enable)
   {
      U32 newButtonMask = 0;
      controls[0] = 0;
      controls[1] = 0;
      controls[2] = 0;
      controls[3] = 0;
      for(U32 i = 0; i < MaxJoystickAxes; i++)
      {
         U32 mask;
         F32 axes1 = axes[i];
         if(axes1 < 0)
         {
            mask = gJoystickMapping.axes[i*2];
            axes1 = -axes1;
         }
         else
            mask = gJoystickMapping.axes[i*2+1];
         if(axes1 > 0.5) newButtonMask |= mask;
         if(mask & 0x10000) controls[0] -= axes1;
         if(mask & 0x20000) controls[0] += axes1;
         if(mask & 0x40000) controls[1] -= axes1;
         if(mask & 0x80000) controls[1] += axes1;
         if(mask & 0x100000) controls[2] -= axes1;
         if(mask & 0x200000) controls[2] += axes1;
         if(mask & 0x400000) controls[3] -= axes1;
         if(mask & 0x800000) controls[3] += axes1;
      }
      newButtonMask &= 0xFF00FFFF;
      for(U32 i = 0; i < 32; i++)
      {
         if(buttonMask & (1 << i))
            newButtonMask |= gJoystickMapping.button[i];
      }
      for(U32 i = 0; i < 4; i++)
      {
         if(hatMask & (ControllerButtonDPadUp << i))
            newButtonMask |= gJoystickMapping.pov[i];
      }
      if(newButtonMask & 0x10000) controls[0] -= 1;
      if(newButtonMask & 0x20000) controls[0] += 1;
      if(newButtonMask & 0x40000) controls[1] -= 1;
      if(newButtonMask & 0x80000) controls[1] += 1;
      if(newButtonMask & 0x100000) controls[2] -= 1;
      if(newButtonMask & 0x200000) controls[2] += 1;
      if(newButtonMask & 0x400000) controls[3] -= 1;
      if(newButtonMask & 0x800000) controls[3] += 1;
      buttonMask = newButtonMask & 0xFF00FFFF;
   }
   else
   {
      controls[0] = axes[0];
      controls[1] = axes[1];
      // Firing input --> controls[2] is left-right, controls[3] is up-down
      controls[2] = axes[shootAxisRemaps[gIniSettings.joystickType][0]];
      controls[3] = axes[shootAxisRemaps[gIniSettings.joystickType][1]];


      // Remap button inputs
      U32 retMask = 0;
      for(U32 i = 0; i < MaxJoystickButtons; i++)
         if(buttonMask & (1 << i))
         {
            retMask |= controllerButtonRemaps[gIniSettings.joystickType][i];
         }
      buttonMask = retMask | hatMask;


      if(gIniSettings.joystickType == XBoxController || gIniSettings.joystickType == XBoxControllerOnXBox && !gJoystickMapping.enable)
      {
         // XBox (windows, not linux) also seems to map triggers to axes[2], so we'll create some pseudo-button events for the triggers here
         // Note that if both triggers are depressed equally, they'll cancel each other out, and if one is pressed more than the other,
         // only that one will be detected.
         F32 deadZone = 0.075f;
         if(axes[2] < -deadZone)
            buttonMask |= ControllerButton7;
         else if(axes[2] > deadZone)
            buttonMask |= ControllerButton8;
      }
   }

   // Movement input --> controls[0] is left-right, controls[1] is up-down
   for(S32 i = 0; i <= 1; i ++)
   {
      //controls[i] = axes[i];

      if(controls[i] < minValues[i])
         minValues[i] = controls[i];
      if(controls[i] > maxValues[i])
         maxValues[i] = controls[i];

      if(controls[i] < 0)
         controls[i] = - (controls[i] / minValues[i]);
      else if(controls[i] > 0)
         controls[i] = (controls[i] / maxValues[i]);

   }

   // XBox control inputs are in a circle, not a square, which makes
   // diagonal movement inputs "slower"
   if(gIniSettings.joystickType == XBoxController || gIniSettings.joystickType == XBoxControllerOnXBox)
   {
      Point dir(controls[0], controls[1]);
      F32 absX = fabs(dir.x);
      F32 absY = fabs(dir.y);

      // Push out to the edge of the square (-1,-1 -> 1,1 )

      F32 dirLen = dir.len() * 1.25;
      if(dirLen > 1)
         dirLen = 1;

      if(absX > absY)
         dir *= F32(dirLen / absX);
      else
         dir *= F32(dirLen / absY);
      controls[0] = dir.x;
      controls[1] = dir.y;
   }



   // Create dead zones, so minimal stick movement or miscalibration will have no effect
   for(U32 i = 0; i < 4; i++)
   {           //              Move      Fire
      F32 deadZone = (i < 2) ? 0.25f : 0.075f;  // Different deadZones for moving and firing.  Why?

      // Recalibrate control entry to compensate for dead zone
      if(controls[i] < -deadZone)
         controls[i] = -(-controls[i] - deadZone) / F32(1 - deadZone);
      else if(controls[i] > deadZone)
         controls[i] = (controls[i] - deadZone) / F32(1 - deadZone);
      else
         controls[i] = 0;                       // If we're in dead zone, set input to 0
   }

   // Save stick input for use later
   gJoystickInput[JOYSTICK_MOVE][AXIS_LEFT_RIGHT] = controls[0];
   gJoystickInput[JOYSTICK_MOVE][AXIS_UP_DOWN] = controls[1];
   gJoystickInput[JOYSTICK_FIRE][AXIS_LEFT_RIGHT] = controls[2];
   gJoystickInput[JOYSTICK_FIRE][AXIS_UP_DOWN] = controls[3];

   // While we're here, we'll create some psuedo key-down events for the
   // joystick, which we'll use primarily for navigating menus and such
   // Note that the effect of these will be digital (either on or off), not analog
   JoystickButtonMask2 = 0;
   if (controls[0] < -0.5) JoystickButtonMask2 |= 1;
   if (controls[0] > 0.5) JoystickButtonMask2 |= 2;
   if (controls[1] < -0.5) JoystickButtonMask2 |= 4;
   if (controls[1] > 0.5) JoystickButtonMask2 |= 8;
   if (controls[2] < -0.5) JoystickButtonMask2 |= 16;
   if (controls[2] > 0.5) JoystickButtonMask2 |= 32;
   if (controls[3] < -0.5) JoystickButtonMask2 |= 64;
   if (controls[3] > 0.5) JoystickButtonMask2 |= 128;

   //logprintf("ButtonMask: %d", buttonMask);


   return true;      // true = processed joystick input
}


extern bool gShowAimVector;

#define absf(x) (((x) > 0) ? (x) : -(x))

static void updateMoveInternal(Move *theMove)
{
   if(gJoystickInput[JOYSTICK_MOVE][AXIS_LEFT_RIGHT] < 0)
   {
      theMove->left = -gJoystickInput[JOYSTICK_MOVE][AXIS_LEFT_RIGHT];
      theMove->right = 0;
   }
   else
   {
      theMove->left = 0;
      theMove->right = gJoystickInput[JOYSTICK_MOVE][AXIS_LEFT_RIGHT];
   }

   if(gJoystickInput[JOYSTICK_MOVE][AXIS_UP_DOWN] < 0)
   {
      theMove->up = -gJoystickInput[JOYSTICK_MOVE][AXIS_UP_DOWN];
      theMove->down = 0;
   }
   else
   {
      theMove->down = gJoystickInput[JOYSTICK_MOVE][AXIS_UP_DOWN];
      theMove->up = 0;
   }


   // Goofball implementation of enableExperimentalAimMode here replicates old behavior when setting is disabled
   Point p(gJoystickInput[JOYSTICK_FIRE][AXIS_LEFT_RIGHT], gJoystickInput[JOYSTICK_FIRE][AXIS_UP_DOWN]);
   F32 plen = p.len();

   F32 maxplen = max(absf(p.x), absf(p.y));

   F32 fact = gIniSettings.enableExperimentalAimMode ? maxplen : plen;

   if(fact > (gIniSettings.enableExperimentalAimMode ? 0.95 : 0.50))    // It requires a large movement to actually fire...
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = true;
      gShowAimVector = true;
   }
   else if(fact > 0.25)   // ...but you can change aim with a smaller one
   {
      theMove->angle = atan2(p.y, p.x);
      theMove->fire = false;
      gShowAimVector = true;
   }
   else
   {
      theMove->fire = false;
      gShowAimVector = false;
   }
}


static bool processJoystickInputJournaled( Move *theMove, U32 &buttonMask )
{
   TNL_JOURNAL_READ_BLOCK(JoystickUpdate,
      BitStream *readStream = Journal::getReadStream();
      if(!readStream->readFlag())
         return false;

      Move aMove;
      aMove.unpack(readStream, false);
      *theMove = aMove;
      buttonMask = readStream->readInt(MaxJoystickButtons);
      return true;
   )

   bool ret = processJoystickInputs(buttonMask);
   if (ret)
      updateMoveInternal(theMove);

   TNL_JOURNAL_WRITE_BLOCK(JoystickUpdate,
      BitStream *writeStream = Journal::getWriteStream();
      if(writeStream->writeFlag(ret))
      {
         Move dummy;
         theMove->pack(writeStream, &dummy, false);
         writeStream->writeInt(buttonMask, MaxJoystickButtons);
      }
   )
   return ret;
}



void simulateKeyDown(KeyCode keyCode)
{
   setKeyState(keyCode, true);
   UserInterface::current->onKeyDown(keyCode, 0);
}

void simulateKeyUp(KeyCode keyCode)
{
   setKeyState(keyCode, false);
   UserInterface::current->onKeyUp(keyCode);
}

// Loads joystick moves into our Move object (overwriting any keyboard input that was already there)
// Also translates joystick button events into our KeyCode system
// Why oh why do we mix our saving of the game-particular move structure with our
//    raw joystick input collection?
U32 JoystickButtonMask2prev = 0;
U32 JoystickLastButtonsPressed = 0;
void JoystickUpdateMove(Move *theMove)
{
   U32 buttonMask;

   if(!processJoystickInputJournaled(theMove, buttonMask))
      return;

   // Figure out which buttons have changed state
   U32 buttonDown = buttonMask & ~JoystickLastButtonsPressed;
   U32 buttonUp = ~buttonMask & JoystickLastButtonsPressed;
   JoystickLastButtonsPressed = buttonMask;

   // Translate button events into our standard keyboard template
   if(buttonDown & ControllerButton1) simulateKeyDown(BUTTON_1);
   if(buttonDown & ControllerButton2) simulateKeyDown(BUTTON_2);
   if(buttonDown & ControllerButton3) simulateKeyDown(BUTTON_3);
   if(buttonDown & ControllerButton4) simulateKeyDown(BUTTON_4);
   if(buttonDown & ControllerButton5) simulateKeyDown(BUTTON_5);
   if(buttonDown & ControllerButton6) simulateKeyDown(BUTTON_6);
   if(buttonDown & ControllerButton7) simulateKeyDown(BUTTON_7);
   if(buttonDown & ControllerButton8) simulateKeyDown(BUTTON_8);
/*   if(buttonDown & ControllerButton9) simulateKeyDown(BUTTON_9);
   if(buttonDown & ControllerButton10) simulateKeyDown(BUTTON_10);
   if(buttonDown & ControllerButton11) simulateKeyDown(BUTTON_11);
   if(buttonDown & ControllerButton12) simulateKeyDown(BUTTON_12);
   if(buttonDown & ControllerButton13) simulateKeyDown(BUTTON_13);
   if(buttonDown & ControllerButton14) simulateKeyDown(BUTTON_14);
   if(buttonDown & ControllerButton15) simulateKeyDown(BUTTON_15); */
   if(buttonDown & ControllerButtonStart) simulateKeyDown(BUTTON_START);
   if(buttonDown & ControllerButtonBack) simulateKeyDown(BUTTON_BACK);
   if(buttonDown & ControllerButtonDPadUp) simulateKeyDown(BUTTON_DPAD_UP);
   if(buttonDown & ControllerButtonDPadDown) simulateKeyDown(BUTTON_DPAD_DOWN);
   if(buttonDown & ControllerButtonDPadLeft) simulateKeyDown(BUTTON_DPAD_LEFT);
   if(buttonDown & ControllerButtonDPadRight) simulateKeyDown(BUTTON_DPAD_RIGHT);

   if(buttonUp & ControllerButton1) simulateKeyUp(BUTTON_1);
   if(buttonUp & ControllerButton2) simulateKeyUp(BUTTON_2);
   if(buttonUp & ControllerButton3) simulateKeyUp(BUTTON_3);
   if(buttonUp & ControllerButton4) simulateKeyUp(BUTTON_4);
   if(buttonUp & ControllerButton5) simulateKeyUp(BUTTON_5);
   if(buttonUp & ControllerButton6) simulateKeyUp(BUTTON_6);
   if(buttonUp & ControllerButton7) simulateKeyUp(BUTTON_7);
   if(buttonUp & ControllerButton8) simulateKeyUp(BUTTON_8);
/*   if(buttonUp & ControllerButton9) simulateKeyUp(BUTTON_9);
   if(buttonUp & ControllerButton10) simulateKeyUp(BUTTON_10);
   if(buttonUp & ControllerButton11) simulateKeyUp(BUTTON_11);
   if(buttonUp & ControllerButton12) simulateKeyUp(BUTTON_12);
   if(buttonUp & ControllerButton13) simulateKeyUp(BUTTON_13);
   if(buttonUp & ControllerButton14) simulateKeyUp(BUTTON_14);
   if(buttonUp & ControllerButton15) simulateKeyUp(BUTTON_15); */
   if(buttonUp & ControllerButtonStart) simulateKeyUp(BUTTON_START);
   if(buttonUp & ControllerButtonBack) simulateKeyUp(BUTTON_BACK);
   if(buttonUp & ControllerButtonDPadUp) simulateKeyUp(BUTTON_DPAD_UP);
   if(buttonUp & ControllerButtonDPadDown) simulateKeyUp(BUTTON_DPAD_DOWN);
   if(buttonUp & ControllerButtonDPadLeft) simulateKeyUp(BUTTON_DPAD_LEFT);
   if(buttonUp & ControllerButtonDPadRight) simulateKeyUp(BUTTON_DPAD_RIGHT);

   buttonDown = JoystickButtonMask2 & ~JoystickButtonMask2prev;
   buttonUp = ~JoystickButtonMask2 & JoystickButtonMask2prev;
   JoystickButtonMask2prev = JoystickButtonMask2;

   if(buttonDown & 1) simulateKeyDown(STICK_1_LEFT);
   if(buttonDown & 2) simulateKeyDown(STICK_1_RIGHT);
   if(buttonDown & 4) simulateKeyDown(STICK_1_UP);
   if(buttonDown & 8) simulateKeyDown(STICK_1_DOWN);
   if(buttonDown & 16) simulateKeyDown(STICK_2_LEFT);
   if(buttonDown & 32) simulateKeyDown(STICK_2_RIGHT);
   if(buttonDown & 64) simulateKeyDown(STICK_2_UP);
   if(buttonDown & 128) simulateKeyDown(STICK_2_DOWN);

   if(buttonUp & 1) simulateKeyUp(STICK_1_LEFT);
   if(buttonUp & 2) simulateKeyUp(STICK_1_RIGHT);
   if(buttonUp & 4) simulateKeyUp(STICK_1_UP);
   if(buttonUp & 8) simulateKeyUp(STICK_1_DOWN);
   if(buttonUp & 16) simulateKeyUp(STICK_2_LEFT);
   if(buttonUp & 32) simulateKeyUp(STICK_2_RIGHT);
   if(buttonUp & 64) simulateKeyUp(STICK_2_UP);
   if(buttonUp & 128) simulateKeyUp(STICK_2_DOWN);
}


};


