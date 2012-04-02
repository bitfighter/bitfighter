/*
 * Input.h
 *
 *  Created on: Jun 12, 2011
 *      Author: noonespecial
 */
#ifndef INPUT_H_
#define INPUT_H_

#include "tnlTypes.h"
#include "InputCode.h"
#include "ConfigEnum.h"

#include "SDL.h"

#if SDL_VERSION_ATLEAST(2,0,0)
#define SDLKey SDL_Keycode
#define SDLMod SDL_Keymod
#endif

using namespace TNL;

namespace Zap {

class ClientGame;

class Event 
{
private:
   static bool mAllowTextInput;   // Flag to allow text translation pass-through

   static void setMousePos(S32 x, S32 y, DisplayMode mode);
   static void updateJoyAxesDirections(U32 axisMask, S16 value);

   static void inputCodeUp(InputCode inputCode);
   static bool inputCodeDown(InputCode inputCode);

   static void onKeyDown(ClientGame *game, SDL_Event *event);
   static void onKeyUp(SDL_Event *event);
   static void onTextInput(char unicode);
   static void onMouseMoved(S32 x, S32 y, DisplayMode mode);
   static void onMouseWheel(bool Up, bool Down);  //Not implemented
   static void onMouseButtonDown(S32 x, S32 y, InputCode inputCode, DisplayMode mode);
   static void onMouseButtonUp(S32 x, S32 y, InputCode inputCode, DisplayMode mode);
   static void onJoyAxis(U8 which, U8 axis, S16 value);
   static void onJoyButtonDown(U8 which, U8 button);
   static void onJoyButtonUp(U8 which, U8 button);
   static void onJoyHat(U8 which, U8 hat, U8 directionMask);
   static void onJoyBall(U8 which, U8 ball, S16 xrel, S16 yrel);
   static void onResize(ClientGame *game, S32 w, S32 h);
   static void onUser(U8 type, S32 code, void* data1, void* data2);

public:
   Event();
   virtual ~Event();

   static void onEvent(ClientGame *game, SDL_Event* event);
};

}
#endif /* INPUT_H_ */
