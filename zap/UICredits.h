//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UICREDITS_H_
#define _UICREDITS_H_

#include "UI.h"

namespace Zap
{


struct CreditsInfo 
{
   Vector<const char *> lines;
   F32 pos;
};


class CreditsScroller
{

public:
   enum Credits {
      MaxCreditLen = 32,
      CreditSpace  = 45,
      SectionSpace = 100,
   };

private:

   Vector<CreditsInfo> mCredits;
   void readCredits(const char *file);
   bool mActivated;

public:
   CreditsScroller();           // Constructor
   virtual ~CreditsScroller();  // Destructor
   void updateFX(U32 delta);
   void render() const;
   void resetPosition();

   void setActive(bool active);
   bool isActive() const;
};

////////////////////////////////////////
////////////////////////////////////////

// Credits UI
class CreditsUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   CreditsScroller *mScroller;

public:
   explicit CreditsUserInterface(ClientGame *game);   // Constructor
   virtual ~CreditsUserInterface();          // Destructor

   void onActivate();
   void onReactivate();
   void idle(U32 timeDelta);
   void render() const;
   void quit();
   bool onKeyDown(InputCode inputCode);
};


////////////////////////////////////////
////////////////////////////////////////
// Splash UI -- provides short animation at startup and at beginning of credits
class SplashUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   enum SplashPhase {
      SplashPhaseNone,
      SplashPhaseAnimation,
      SplashPhaseResting,
      SplashPhaseRising,
      SplashPhaseDone
   };

   Timer mSplashTimer;    // Timer controlling progress through the phase
   SplashPhase mPhase;            // Phase of the animation

public:
   explicit SplashUserInterface(ClientGame *game);      // Constructor
   virtual ~SplashUserInterface();

   void onActivate();
   void idle(U32 timeDelta);
   void render() const;
   void quit();
   bool onKeyDown(InputCode inputCode);
};

}

#endif

