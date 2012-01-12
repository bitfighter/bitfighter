/*
 * Input.cpp
 *
 *  Created on: Jun 12, 2011
 *      Author: noonespecial
 */

#include "Event.h"

#include "oglconsole.h"
#include "config.h"
#include "UIMenus.h"
#include "UIDiagnostics.h"
#include "IniFile.h"
#include "ScreenShooter.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "ClientGame.h"

#include "SDL/SDL_opengl.h"

#include <cmath>

namespace Zap
{

Event::Event()
{
   // Do nothing -- never called?
}


Event::~Event()
{
   // Do nothing
}


void Event::setMousePos(S32 x, S32 y, DisplayMode reportedDisplayMode)
{
   // Handle special case of editor... would be better handled elsewhere?
   if(UserInterface::current->getMenuID() == EditorUI && reportedDisplayMode == DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED)
      reportedDisplayMode = DISPLAY_MODE_FULL_SCREEN_STRETCHED;

   gScreenInfo.setMousePos(x, y, reportedDisplayMode);
}


// Argument of axisMask is one of the 4 axes:
//    MoveAxisLeftRightMask, MoveAxisUpDownMask, ShootAxisLeftRightMask, ShootAxisUpDownMask
void Event::updateJoyAxesDirections(U32 axisMask, S16 value)
{
   // Get our current joystick-axis-direction and its opposite on the same axis
   U32 detectedAxesDirectionMask = 0;
   U32 oppositeDetectedAxesDirectionMask = 0;
   if (value < 0)
   {
      detectedAxesDirectionMask = axisMask & NegativeAxesMask;
      oppositeDetectedAxesDirectionMask = axisMask & PositiveAxesMask;
   }
   else
   {
      detectedAxesDirectionMask = axisMask & PositiveAxesMask;
      oppositeDetectedAxesDirectionMask = axisMask & NegativeAxesMask;
   }

   // Get the specific axes direction index (and opposite) from the mask we've detected
   // from enum JoystickAxesDirections
   U32 axesDirectionIndex = 0;
   U32 oppositeAxesDirectionIndex = 0;
   for (S32 i = 0; i < MaxAxesDirections; i++)
   {
      if(Joystick::JoystickInputData[i].axesMask & detectedAxesDirectionMask)
         axesDirectionIndex = i;
      if(Joystick::JoystickInputData[i].axesMask & oppositeDetectedAxesDirectionMask)
         oppositeAxesDirectionIndex = i;
   }


   // Update our global joystick input data, use a sensitivity threshold to take care of calibration issues
   // Also, normalize the input value to a floating point scale of 0 to 1
   F32 normalValue;
   S32 absValue = abs(value);
   if(absValue < Joystick::LowerSensitivityThreshold)
      normalValue = 0.0f;
   else if(absValue > Joystick::UpperSensitivityThreshold)
      normalValue = 1.0f;
   else
      normalValue = (F32)(absValue - Joystick::LowerSensitivityThreshold) / (F32)(Joystick::UpperSensitivityThreshold - Joystick::LowerSensitivityThreshold);

//   logprintf("value: %d;\tnormalValue: %f", value, normalValue);

   Joystick::JoystickInputData[axesDirectionIndex].value = normalValue;


   // Now set the opposite axis back to zero
   Joystick::JoystickInputData[oppositeAxesDirectionIndex].value = 0.0f;


   // Determine what to set the InputCode state, it is binary so set the threshold has half -> 0.5
   // Set the mask if it is above the digital threshold
   U32 currentInputCodeMask = 0;

   for (S32 i = 0; i < MaxAxesDirections; i++)
      if (fabs(Joystick::JoystickInputData[i].value) > 0.5)
         currentInputCodeMask |= (1 << i);


   // Only change InputCode state if the axis has changed.  Time to be tricky..
   U32 inputCodeDownDeltaMask = currentInputCodeMask & ~Joystick::AxesInputCodeMask;
   U32 inputCodeUpDeltaMask = ~currentInputCodeMask & Joystick::AxesInputCodeMask;

   for (S32 i = 0; i < MaxAxesDirections; i++)
   {
      // If the current axes direction is set in the inputCodeDownDeltaMask, set the input code down
      if (Joystick::JoystickInputData[i].axesMask & inputCodeDownDeltaMask)
      {
         inputCodeDown(Joystick::JoystickInputData[i].inputCode, 0);
         continue;
      }

      // If the current axes direction is set in the inputCodeUpDeltaMask, set the input code up
      if (Joystick::JoystickInputData[i].axesMask & inputCodeUpDeltaMask)
         inputCodeUp(Joystick::JoystickInputData[i].inputCode);
   }


   // Finally alter the global axes InputCode mask to reflect the current inputCodeState
   Joystick::AxesInputCodeMask = currentInputCodeMask;
}


void Event::inputCodeUp(InputCode inputCode)
{
   setInputCodeState(inputCode, false);

   if(UserInterface::current)
      UserInterface::current->onKeyUp(inputCode);
}


void Event::inputCodeDown(InputCode inputCode, char ascii)
{
   setInputCodeState(inputCode, true);

   if(UserInterface::current)
      UserInterface::current->onKeyDown(inputCode, ascii);
}


void pushSimulatedAltKeyPress()
{
   SDL_Event evnt;

   evnt.type = 0x02;

   evnt.active.gain = 0;
   evnt.active.state = 0x01;
   evnt.active.type = 0x02;

   evnt.key.type = 0x02;
   evnt.key.which = 0;
   evnt.key.state = 0x01;
   evnt.key.keysym.mod = KMOD_NONE;
   evnt.key.keysym.scancode = 0x38;
   evnt.key.keysym.sym = SDLK_LALT;
   evnt.key.keysym.unicode = 0;

      //SDL_PushEvent(&evnt);
   

      // Try leaving the rest at default value



//-		motion	{type='' which=0x00 state='' ...}	SDL_MouseMotionEvent
//		type	0x02 ''	unsigned char
//		which	0x00	unsigned char
//		state	0x01 ''	unsigned char
//		x	0x0238	unsigned short
//		y	0x0000	unsigned short
//		xrel	0x0134	short
//		yrel	0x0000	short
//-		button	{type='' which=0x00 button='' ...}	SDL_MouseButtonEvent
//		type	0x02 ''	unsigned char
//		which	0x00	unsigned char
//		button	0x01 ''	unsigned char
//		state	0x00	unsigned char
//		x	0x0238	unsigned short
//		y	0x0000	unsigned short
//-		jaxis	{type='' which=0x00 axis='' ...}	SDL_JoyAxisEvent
//		type	0x02 ''	unsigned char
//		which	0x00	unsigned char
//		axis	0x01 ''	unsigned char
//		value	0x0238	short
//-		jball	{type='' which=0x00 ball='' ...}	SDL_JoyBallEvent
//		type	0x02 ''	unsigned char
//		which	0x00	unsigned char
//		ball	0x01 ''	unsigned char
//		xrel	0x0238	short
//		yrel	0x0000	short
//-		jhat	{type='' which=0x00 hat='' ...}	SDL_JoyHatEvent
//		type	0x02 ''	unsigned char
//		which	0x00	unsigned char
//		hat	0x01 ''	unsigned char
//		value	0x00	unsigned char
//-		jbutton	{type='' which=0x00 button='' ...}	SDL_JoyButtonEvent
//		type	0x02 ''	unsigned char
//		which	0x00	unsigned char
//		button	0x01 ''	unsigned char
//		state	0x00	unsigned char
//-		resize	{type='' w=0x00000238 h=0x00000134 }	SDL_ResizeEvent
//		type	0x02 ''	unsigned char
//		w	0x00000238	int
//		h	0x00000134	int
//-		expose	{type='' }	SDL_ExposeEvent
//		type	0x02 ''	unsigned char
//-		quit	{type='' }	SDL_QuitEvent
//		type	0x02 ''	unsigned char
//-		user	{type='' code=0x00000238 data1=0x00000134 ...}	SDL_UserEvent
//		type	0x02 ''	unsigned char
//		code	0x00000238	int
//		data1	0x00000134	void *
//		data2	0x00000000	void *
//-		syswm	{type='' msg=0x00000238 }	SDL_SysWMEvent
//		type	0x02 ''	unsigned char
//-		msg	0x00000238 {version={...} hwnd=??? msg=??? ...}	SDL_SysWMmsg *
//+		version	{major=??? minor=??? patch=??? }	SDL_version
//		hwnd	CXX0017: Error: symbol "" not found	
//		msg	CXX0030: Error: expression cannot be evaluated	
//		wParam	CXX0030: Error: expression cannot be evaluated	
//		lParam	CXX0030: Error: expression cannot be evaluated	

}


void Event::onEvent(ClientGame *game, SDL_Event* event)
{
   switch (event->type)
   {
      case SDL_KEYDOWN:
         onKeyDown(game, event->key.keysym.sym, event->key.keysym.mod, event->key.keysym.unicode);
         break;

      case SDL_KEYUP:
         onKeyUp(event->key.keysym.sym, event->key.keysym.mod, event->key.keysym.unicode);
         break;

      case SDL_MOUSEMOTION:
            onMouseMoved(event->motion.x, event->motion.y, game->getSettings()->getIniSettings()->displayMode);
         break;

      case SDL_MOUSEBUTTONDOWN:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonDown(event->button.x, event->button.y, MOUSE_LEFT, game->getSettings()->getIniSettings()->displayMode);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonDown(event->button.x, event->button.y, MOUSE_RIGHT, game->getSettings()->getIniSettings()->displayMode);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonDown(event->button.x, event->button.y, MOUSE_MIDDLE, game->getSettings()->getIniSettings()->displayMode);
               break;
            case SDL_BUTTON_WHEELUP:
               onMouseWheel(true, false);
               break;
            case SDL_BUTTON_WHEELDOWN:
               onMouseWheel(false, true);
               break;
         }
         break;

      case SDL_MOUSEBUTTONUP:
         switch (event->button.button)
         {
            case SDL_BUTTON_LEFT:
               onMouseButtonUp(event->button.x, event->button.y, MOUSE_LEFT, game->getSettings()->getIniSettings()->displayMode);
               break;

            case SDL_BUTTON_RIGHT:
               onMouseButtonUp(event->button.x, event->button.y, MOUSE_RIGHT, game->getSettings()->getIniSettings()->displayMode);
               break;

            case SDL_BUTTON_MIDDLE:
               onMouseButtonUp(event->button.x, event->button.y, MOUSE_MIDDLE, game->getSettings()->getIniSettings()->displayMode);
               break;
         }
         break;

      case SDL_JOYAXISMOTION:
         onJoyAxis(event->jaxis.which, event->jaxis.axis, event->jaxis.value);
         break;

         // TODO:  Do we need joyball?
      case SDL_JOYBALLMOTION:
         onJoyBall(event->jball.which, event->jball.ball, event->jball.xrel, event->jball.yrel);
         break;

      case SDL_JOYHATMOTION:
         onJoyHat(event->jhat.which, event->jhat.hat, event->jhat.value);
         break;

      case SDL_JOYBUTTONDOWN:
         onJoyButtonDown(event->jbutton.which, event->jbutton.button);
         break;

      case SDL_JOYBUTTONUP:
         onJoyButtonUp(event->jbutton.which, event->jbutton.button);
         break;

      case SDL_SYSWMEVENT:
         //Ignore
         break;

      case SDL_VIDEORESIZE:
         onResize(game, event->resize.w, event->resize.h);
         break;

      case SDL_VIDEOEXPOSE:
         onExpose();
         break;

      case SDL_ACTIVEEVENT:
         switch (event->active.state)
         {
            case SDL_APPMOUSEFOCUS:
               if (event->active.gain)
                  onMouseFocus();
               else
                  onMouseBlur();

               break;

            case SDL_APPINPUTFOCUS:
               if (event->active.gain)
                  onInputFocus();
               else
                  onInputBlur();

               break;

            case SDL_APPACTIVE:
               if (event->active.gain)
                  onRestore();
               else
                  onMinimize();

               break;
         }
         break;

      default:
         onUser(event->user.type, event->user.code, event->user.data1, event->user.data2);
         break;
   }
}

void Event::onInputFocus()
{
   // Do nothing
}


void Event::onInputBlur()     // <=== what does this do??
{
   // Do nothing
}


void Event::onKeyDown(ClientGame *game, SDLKey key, SDLMod mod, U16 unicode)
{
   // Use getInputCodeState() instead of checking the mod flag to prevent hyper annoying case
   // of user pressing and holding Alt, selecting another window, releasing Alt, returning to
   // Bitfighter window, and pressing enter, and having this block think we pressed alt-enter.
   // GetKeyState() looks at the actual current state of the key, which is what we want.

   // ALT+ENTER --> toggles window mode/full screen
   if(key == SDLK_RETURN && (SDL_GetModState() & KMOD_ALT))
   {
      const Point *pos = gScreenInfo.getMousePos();

      game->getUIManager()->getOptionsMenuUserInterface()->toggleDisplayMode();
      //pushSimulatedAltKeyPress();

      gScreenInfo.setCanvasMousePos(pos->x, pos->y, game->getSettings()->getIniSettings()->displayMode);

      SDL_WarpMouse(gScreenInfo.getWindowMousePos()->x, gScreenInfo.getWindowMousePos()->y);
   }

   // CTRL+Q --> screenshot!
   else if(key == SDLK_q && getInputCodeState(KEY_CTRL))
      ScreenShooter::saveScreenshot(game->getSettings()->getFolderManager()->screenshotDir);

   // The rest
   else
   {
      InputCode inputCode = sdlKeyToInputCode(key);
      inputCodeDown(inputCode, keyToAscii(unicode, inputCode));
   }
}


void Event::onKeyUp(SDLKey key, SDLMod mod, U16 unicode)
{
   inputCodeUp(sdlKeyToInputCode(key));
}


void Event::onMouseFocus()
{
   // Do nothing
}


void Event::onMouseBlur()
{
   // Do nothing
}


void Event::onMouseMoved(S32 x, S32 y, DisplayMode mode)
{
   setMousePos(x, y, mode);

   if(UserInterface::current)
      UserInterface::current->onMouseMoved();
}


void Event::onMouseWheel(bool Up, bool Down)
{
   if(Up)
   {
      inputCodeDown(MOUSE_WHEEL_UP);
      inputCodeUp(MOUSE_WHEEL_UP);
   }
   if(Down)
   {
      inputCodeDown(MOUSE_WHEEL_DOWN);
      inputCodeUp(MOUSE_WHEEL_DOWN);
   }
   // Do nothing
}


void Event::onMouseButtonDown(S32 x, S32 y, InputCode inputCode, DisplayMode mode)
{
   setMousePos(x, y, mode);

   inputCodeDown(inputCode, 0);
}


void Event::onMouseButtonUp(S32 x, S32 y, InputCode inputCode, DisplayMode mode)
{
   setMousePos(x, y, mode);

   inputCodeUp(inputCode);
}


void Event::onJoyAxis(U8 whichJoystick, U8 axis, S16 value)
{
//   logprintf("SDL Axis number: %u, value: %d", axis, value);

   if(axis < Joystick::rawAxisCount)
      Joystick::rawAxis[axis] = (F32)value / (F32)S16_MAX;

   // Left/Right movement axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].moveAxesSdlIndex[0])
      updateJoyAxesDirections(MoveAxisLeftRightMask,  value);

   // Up/down movement axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].moveAxesSdlIndex[1])
      updateJoyAxesDirections(MoveAxisUpDownMask,  value);

   // Left/Right shooting axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].shootAxesSdlIndex[0])
      updateJoyAxesDirections(ShootAxisLeftRightMask, value);

   // Up/down shooting axis
   if(axis == Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].shootAxesSdlIndex[1])
      updateJoyAxesDirections(ShootAxisUpDownMask, value);
}


extern InputCode joystickButtonToInputCode(Joystick::Button button);

void Event::onJoyButtonDown(U8 which, U8 button)
{
//   logprintf("SDL button down number: %u", button);
   inputCodeDown(joystickButtonToInputCode(Joystick::remapSdlButtonToJoystickButton(button)), 0);
   Joystick::ButtonMask |= BIT(button);
}


void Event::onJoyButtonUp(U8 which, U8 button)
{
//   logprintf("SDL button up number: %u", button);
   inputCodeUp(joystickButtonToInputCode(Joystick::remapSdlButtonToJoystickButton(button)));
   Joystick::ButtonMask = Joystick::ButtonMask & ~BIT(button);
}


extern InputCode joyHatToInputCode(int hatDirectionMask);

// See SDL_Joystick.h for the SDL_HAT_* mask definitions
void Event::onJoyHat(U8 which, U8 hat, U8 directionMask)
{
//   logprintf("SDL Hat number: %u, value: %u", hat, directionMask);

   InputCode inputCode;
   U32 inputCodeDownDeltaMask = directionMask & ~Joystick::HatInputCodeMask;
   U32 inputCodeUpDeltaMask = ~directionMask & Joystick::HatInputCodeMask;

   for (S32 i = 0; i < MaxHatDirections; i++)
   {
      inputCode = joyHatToInputCode(BIT(i));  // BIT(i) corresponds to a defined SDL_HAT_*  value

      // If the current hat direction is set in the inputCodeDownDeltaMask, set the input code down
      if (inputCodeDownDeltaMask & BIT(i))
         inputCodeDown(inputCode, 0);

      // If the current hat direction is set in the inputCodeUpDeltaMask, set the input code up
      if (inputCodeUpDeltaMask & BIT(i))
         inputCodeUp(inputCode);
   }

   // Finally alter the global hat InputCode mask to reflect the current input code State
   Joystick::HatInputCodeMask = directionMask;
}

void Event::onJoyBall(U8 which, U8 ball, S16 xrel, S16 yrel)
{
//   logprintf("SDL Ball number: %u, relative x: %d, relative y: %d", ball, xrel, yrel);
}


void Event::onMinimize()
{
   // Do nothing
}


void Event::onRestore()
{
   // Do nothing
}


// We don't need to worry about this event in fullscreen modes because it is never fired with SDL
void Event::onResize(ClientGame *game, S32 width, S32 height)
{
   S32 canvasHeight = gScreenInfo.getGameCanvasHeight();
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   // Constrain window to correct proportions...
   if((width - canvasWidth) > (height - canvasHeight))      // Wider than taller  (is this right? mixing virtual and physical pixels)
      game->getSettings()->getIniSettings()->winSizeFact = max((F32) height / (F32)canvasHeight, gScreenInfo.getMinScalingFactor());
   else
      game->getSettings()->getIniSettings()->winSizeFact = max((F32) width / (F32)canvasWidth, gScreenInfo.getMinScalingFactor());

   S32 newWidth  = (S32)floor(canvasWidth  * game->getSettings()->getIniSettings()->winSizeFact + 0.5f);   // virtual * (physical/virtual) = physical, fix rounding problem
   S32 newHeight = (S32)floor(canvasHeight * game->getSettings()->getIniSettings()->winSizeFact + 0.5f);

   S32 flags = 0;
   flags = SDL_OPENGL | SDL_RESIZABLE;
   SDL_SetVideoMode(newWidth, newHeight, 0, flags);
   gScreenInfo.setWindowSize(newWidth, newHeight);
  
   glViewport(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());
   OGLCONSOLE_Reshape();

   gINI.SetValueF("Settings", "WindowScalingFactor", game->getSettings()->getIniSettings()->winSizeFact, true);

   glScissor(0, 0, gScreenInfo.getWindowWidth(), gScreenInfo.getWindowHeight());    // See comment on identical line in main.cpp
}


void Event::onExpose()
{
   // Do nothing
}

void Event::onUser(U8 type, S32 code, void* data1, void* data2)
{

}

}
