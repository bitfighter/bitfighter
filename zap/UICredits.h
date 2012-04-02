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

#ifndef _UICREDITS_H_
#define _UICREDITS_H_

#include "UI.h"
#include "game.h"

namespace Zap
{

class CreditsFX
{
protected:
   bool activated;

public:
   CreditsFX(ClientGame *game);     // Constructor
   virtual ~CreditsFX();            // Destructor
   
   void setActive(bool active);
   bool isActive();
   virtual void updateFX(U32 delta) = 0;
   virtual void render() = 0;
};


struct CreditsInfo 
{
   Vector<const char *> creditsLine;
   F32 pos;

   CreditsInfo();    // Constructor
};


class CreditsScroller : public CreditsFX
{
   typedef CreditsFX Parent;

public:
   enum Credits {
      MaxCreditLen = 32,
      CreditSpace  = 45,
   };

private:

   Vector<CreditsInfo> credits;
   void readCredits(const char *file);
   F32 mTotalSize;

public:
   CreditsScroller(ClientGame *game);      // Constructor
   virtual ~CreditsScroller();             // Destructor
   void updateFX(U32 delta);
   void render();
};

// Credits UI
class CreditsUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   Vector<CreditsFX *> fxList;

public:
   CreditsUserInterface(ClientGame *game);   // Constructor
   virtual ~CreditsUserInterface();          // Destructor

   void onActivate();
   void onReactivate();
   void addFX(CreditsFX *fx);
   void idle(U32 timeDelta);
   void render();
   void quit();
   bool onKeyDown(InputCode inputCode);
};


////////////////////////////////////////
////////////////////////////////////////
// Splash UI -- provides short animation at startup and at beginning of credits
class SplashUserInterface : public UserInterface
{
   typedef UserInterface Parent;

   enum {                // All times in ms
     spinTime = 1500,
     restTime = 150,
     riseTime = 800,
  };

private:
   Timer mSplashTimer;    // Timer controlling progress through the phase
   S32 mPhase;            // Phase of the animation
   S32 mType;             // Type of animation (twirl, zoom out, etc.)

public:
   SplashUserInterface(ClientGame *game);      // Constructor

   void onActivate();
   void idle(U32 timeDelta);
   void render();
   void quit();
   bool onKeyDown(InputCode inputCode);
};

}

#endif

