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

#include "UICredits.h"
#include "UIMenus.h"
#include "gameObjectRender.h"    // For renderBitfighterLogo
#include "ClientGame.h"

#include "../tnl/tnlRandom.h"
#include "ScreenInfo.h"

#include "OpenglUtils.h"

#include <stdio.h>
#include <math.h>

namespace Zap
{

static const char *gameCredits[] = {
   "Developed by:",
   "Chris Eykamp (watusimoto)",
   "David Buck (raptor)",
   "Samuel Williams (sam686)",
   "-",
   "Originally based on the Zap demo in OpenTNL",
   "-",
   "Mac support:",
   "Vittorio Giovara (koda)",
   "Ryan Witmer",
   "Max Hushahn",
   "-",
   "Linux support:",
   "David Buck (raptor)",
   "Coding_Mike",
   "Janis Rucis",
   "-",
   "Level contributions:",
   "Qui",
   "Pierce Youatt (karamazovapy)",
   "-",
   "Bot development:",
   "Samuel Williams (sam686)",
   "Joseph Ivie",
   "-",
   "Testing and ideas:",
   "Pierce Youatt (karamazovapy)",
   "Jonathan Hansen",
   "-",
   "Music:",
   "vovk50",
   "-",
   "Get the latest",
   "at",
   "Bitfighter.org",
   "-",                    // Need to end with this...
   NULL                    // ...then this
};


static bool quitting = false;

// Constructor
CreditsUserInterface::CreditsUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(CreditsUI);
}


// Destructor (only gets run when we quit the game!)
CreditsUserInterface::~CreditsUserInterface()
{
   for(S32 i = 0; i < fxList.size(); i++)
   {
      delete fxList[i];
      fxList[i] = NULL;
   }

   fxList.clear();
}


void CreditsUserInterface::onActivate()
{
   quitting = false;
   getUIManager()->getSplashUserInterface()->activate();          // Show splash animation at beginning of credits

   // Construct the creditsfx objects here, they will
   // get properly deleted when the CreditsUI
   // destructor is invoked

   // Add credits scroller first and make it active
   CreditsScroller *scroller = new CreditsScroller(getGame());    // Constructor adds this to fxList
   scroller->setActive(true);

   if(fxList.size() > 1)
      for(S32 i = 0; i < fxList.size(); i++)
         fxList[i]->setActive(false);

   // Add CreditsFX effects below here, don't activate:

   // Choose randomly another CreditsFX effect aside from CreditsScroller to activate
   if(fxList.size() > 1)
   {
      U32 rand = TNL::Random::readI(0, fxList.size() - 1);
      while(fxList[rand]->isActive())
         rand = TNL::Random::readI(0, fxList.size() - 1);

      fxList[rand]->setActive(true);
   }
}


void CreditsUserInterface::onReactivate()
{
   if(quitting)
      quit();     
}


void CreditsUserInterface::addFX(CreditsFX *fx)
{
   fxList.push_back(fx);
}


void CreditsUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   for(S32 i = 0; i < fxList.size(); i++)
      if(fxList[i]->isActive())
         fxList[i]->updateFX(timeDelta);
}


void CreditsUserInterface::render()
{
   // loop through all the attached effects and
   // call their render function
   for(S32 i = 0; i < fxList.size(); i++)
      if(fxList[i]->isActive())
         fxList[i]->render();

   if(quitting)
   {
      quitting = false;
      quit();
   }
}


void CreditsUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();      // gMainMenuUserInterface
}


bool CreditsUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;
   else
      quit();     // Quit the interface when any key is pressed...  any key at all.  Except those handled by Parent.

   return false;
}

//-----------------------------------------------------
// CreditsFX Objects
//-----------------------------------------------------

// Constructor
CreditsFX::CreditsFX(ClientGame *game)
{
   activated = false;
   game->getUIManager()->getCreditsUserInterface()->addFX(this);
}


// Destructor
CreditsFX::~CreditsFX()
{
   // Do nothing
}


void CreditsFX::setActive(bool active)
{
   activated = active;
}


bool CreditsFX::isActive()
{
   return activated;
}


// Constructor
CreditsScroller::CreditsScroller(ClientGame *game) : Parent(game)
{
   glLineWidth(gDefaultLineWidth);

   // Loop through each line in the credits looking for section breaks ("-")
   // thus creating groups, the first of which is generally the job, followed
   // by 0 or more people doing that job

   CreditsInfo c;    // Reusable object, gets implicitly copied when pushed

   static const S32 SPACE_BETWEEN_SECTIONS = 100;

   S32 index = 0;
   while(gameCredits[index])
   {
      if(strcmp(gameCredits[index], "-"))
         c.creditsLine.push_back(gameCredits[index]);

      else   // Place credit in cache
      {
         credits.push_back(c);
         c.pos += CreditSpace * c.creditsLine.size() + SPACE_BETWEEN_SECTIONS;
         c.creditsLine.clear();
      }

      index++;
   }

   mTotalSize = c.pos;
}


CreditsScroller::~CreditsScroller()       // Destructor
{
   // Do nothing
}


void CreditsScroller::updateFX(U32 delta)
{
   // Scroll the credits text from bottom to top
   for(S32 i = 0; i < credits.size(); i++)
      credits[i].pos -= (delta / 8.f);

   // If we've reached the end, say we're quitting
   if(credits[credits.size() - 1].pos < -CreditSpace)
      quitting = true;
}


void CreditsScroller::render()
{
   glColor(Colors::white);

   // Draw the credits text, section by section, line by line
   for(S32 i = 0; i < credits.size(); i++)
      for(S32 j = 0; j < credits[i].creditsLine.size(); j++)
         UserInterface::drawCenteredString(S32(credits[i].pos) + CreditSpace * (j), 25, credits[i].creditsLine[j]);

   glColor(Colors::black);
   F32 vertices[] = {
         0, 0,
         0, 150,
         gScreenInfo.getGameCanvasWidth(), 150,
         gScreenInfo.getGameCanvasWidth(), 0
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_TRIANGLE_FAN);

   renderStaticBitfighterLogo();    // And add our logo at the top of the page
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
CreditsInfo::CreditsInfo()
{
   pos = (F32)gScreenInfo.getGameCanvasHeight();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SplashUserInterface::SplashUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(SplashUI);
}


void SplashUserInterface::onActivate()
{
   mSplashTimer.reset(spinTime);
   glLineWidth(gDefaultLineWidth);

   mPhase = 1;    // Start in the first phase, of course
   mType = rand() % 3 + 1;     // 1 = twirl, 2 = zoom in, 3 = single letters

   mType = 3;     // For now, we'll stick with this one, as it's the best!
}


void SplashUserInterface::idle(U32 timeDelta)
{
   Parent::idle(timeDelta);

   if(mSplashTimer.update(timeDelta))
   {
      mPhase++;
      if(mPhase == 2)
         mSplashTimer.reset(restTime);
      else if(mPhase == 3)
         mSplashTimer.reset(riseTime);
   }

   if(mPhase > 3)
      quit();
}


void SplashUserInterface::render()
{
   if(mPhase == 1)            // Main animation phase
   {
      glColor(0, mSplashTimer.getFraction(), 1);

      if(mType == 1)          // Twirl - unused?   arguments might be wrong..
         renderBitfighterLogo(gScreenInfo.getGameCanvasHeight() / 2, 
                             (1 - mSplashTimer.getFraction()), U32((1 - mSplashTimer.getFraction()) * 360.0f));
      else if(mType == 2)     // Zoom in - unused?   arguments might be wrong..
         renderBitfighterLogo(gScreenInfo.getGameCanvasHeight() / 2, 
                              1 + pow(mSplashTimer.getFraction(), 2) * 20.0f, U32(mSplashTimer.getFraction() * 20));
      else if(mType == 3)     // Single letters
      {
         F32 fr = pow(mSplashTimer.getFraction(), 2);

         S32 ctr = gScreenInfo.getGameCanvasHeight() / 2;

         renderBitfighterLogo(ctr, fr * 20.0f + 1, 1 << 0);
         renderBitfighterLogo(ctr, fr * 50.0f + 1, 1 << 1);
         renderBitfighterLogo(ctr, fr * 10.0f + 1, 1 << 2);
         renderBitfighterLogo(ctr, fr *  2.0f + 1, 1 << 3);
         renderBitfighterLogo(ctr, fr * 14.0f + 1, 1 << 4);
         renderBitfighterLogo(ctr, fr *  6.0f + 1, 1 << 5);
         renderBitfighterLogo(ctr, fr * 33.0f + 1, 1 << 6);
         renderBitfighterLogo(ctr, fr *  9.0f + 1, 1 << 7);
         renderBitfighterLogo(ctr, fr * 30.0f + 1, 1 << 8);
         renderBitfighterLogo(ctr, fr * 15.0f + 1, 1 << 9);
      }
   }
   else if(mPhase == 2)           // Resting phase
   {
      glColor(Colors::blue);
      renderBitfighterLogo(gScreenInfo.getGameCanvasHeight() / 2, 1);
   }
   else if(mPhase == 3)           // Rising phase
   {
      glColor(0, sqrt(1 - mSplashTimer.getFraction()), 1 - pow(1 - mSplashTimer.getFraction(), 2));
      renderBitfighterLogo((S32)(73.0f + ((F32) gScreenInfo.getGameCanvasHeight() / 2.0f - 73.0f) * mSplashTimer.getFraction()), 1);
   }
}


void SplashUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();      //gMainMenuUserInterface
}


bool SplashUserInterface::onKeyDown(InputCode inputCode)
{
   if(!Parent::onKeyDown(inputCode))
   {

      quitting = true;
      quit();                              // Quit the interface when any key is pressed...  any key at all.  Almost.

      // Unless user hit Enter or Escape, or some other thing...
      if(inputCode != KEY_ESCAPE && inputCode != KEY_ENTER && inputCode != MOUSE_LEFT && inputCode != MOUSE_MIDDLE && inputCode != MOUSE_RIGHT)    
         current->onKeyDown(inputCode);                // ...pass keystroke on  (after reactivate in quit(), current is now the underlying

      if(inputCode == MOUSE_LEFT && inputCode == MOUSE_MIDDLE && inputCode == MOUSE_RIGHT)
         current->onMouseMoved();
   }

   return true;
}


};


