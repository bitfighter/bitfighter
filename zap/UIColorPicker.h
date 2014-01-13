//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UICOLORPICKER_H_
#define _UICOLORPICKER_H_

#include "Color.h"
#include "UI.h"

namespace Zap
{


class UIColorPicker : public UserInterface, public Color
{
   typedef UserInterface Parent;

   U32 mMouseDown;

public:
   explicit UIColorPicker(ClientGame *game);    // Constructor
   virtual ~UIColorPicker();                    // Destructor

   void onActivate();
   void onReactivate();
   void idle(U32 timeDelta);
   void render();
   void quit();
   bool onKeyDown(InputCode inputCode);
   void onKeyUp(InputCode inputCode);
   void onMouseMoved();
};

}

#endif

