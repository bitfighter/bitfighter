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

#include "../tnl/tnlRandom.h"
#include "ScreenInfo.h"

#include "SDL/SDL_opengl.h"

#include <stdio.h>
#include <math.h>

namespace Zap
{

const char *gGameCredits[] = {
   "developed by Chris Eykamp (Watusimoto)",
   "Originally based on Zap demo in OpenTNL",
   "-",
   "Mac support:",
   "Ryan Witmer",
   "Max Hushahn",
   "-",
   "Linux support:",
   "Coding_Mike",
   "Janis Rucis",
   "-",
   "Level contributions:",
   "Qui",
   "-",
   "Bot development:",
   "Joseph Ivie",
   "-",
   "Testing and ideas:",
   "Jonathan Hansen",
   "Samuel Williams (sam686)",
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


// Destructor
CreditsUserInterface::~CreditsUserInterface()
{
   for(S32 i = 0; i < fxList.size(); i++)
   {
      delete fxList[i];
      fxList[i] = NULL;
      fxList.erase(i);
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
   CreditsScroller *scroller = new CreditsScroller(getGame());
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
      {
         rand = TNL::Random::readI(0, fxList.size() - 1);
      }
      fxList[rand]->setActive(true);
   }
}


void CreditsUserInterface::onReactivate()
{
   if(quitting)
      quit();     
}


void CreditsUserInterface::idle(U32 timeDelta)
{
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
}


void CreditsUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();      // gMainMenuUserInterface
}


void CreditsUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
      quit();     // Quit the interface when any key is pressed...  any key at all.
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

// Constructor
CreditsScroller::CreditsScroller(ClientGame *game) : Parent(game)
{
   glLineWidth(gDefaultLineWidth);

   // Loop through each line in the credits looking for section breaks ("-")
   // thus creating groups, the first of which is generally the job, followed
   // by 0 or more people doing that job

   S32 pos = (gScreenInfo.getGameCanvasHeight() + CreditSpace);
   S32 index = 0;
   CreditsInfo c;

   while(gGameCredits[index])
   {
      if(strcmp(gGameCredits[index], "-"))
         c.creditsLine.push_back(gGameCredits[index]);
      else   // Place credit in cache
      {
         c.currPos.x = (F32)pos;
         pos += CreditGap;
         credits.push_back(c);
         c.creditsLine.clear();
      }

      pos += CreditSpace;     // Should this go in the if/else construct above?
      index++;
   }
   mTotalSize = pos;
}


void CreditsScroller::updateFX(U32 delta)
{
   // Scroll the credits text from bottom to top
   for(S32 i = 0; i < credits.size(); i++)
   {
      credits[i].currPos.x -= (delta / 8.f);
      S32 pos = S32( credits[i].currPos.x );

      // Reached the top, reset
      if(pos < -CreditSpace )
         pos += mTotalSize - 350;

   }
}


void CreditsScroller::render()
{
   glColor3f(1,1,1);

   // Draw the credits text, section by section, line by line
   for(S32 i = 0; i < credits.size(); i++)
   {
      for(S32 j = 0; j < credits[i].creditsLine.size(); j++)
         UserInterface::drawCenteredString(S32(credits[i].currPos.x) + CreditSpace*(j + 1), 25, credits[i].creditsLine[j]);
   }

   glColor3f(0, 0, 0);
   glBegin(GL_POLYGON);
      glVertex2i(0, 0);
      glVertex2i(0, 150);
      glVertex2i(gScreenInfo.getGameCanvasWidth(), 150);
      glVertex2i(gScreenInfo.getGameCanvasWidth(), 0);
   glEnd();

   renderStaticBitfighterLogo();    // And add our logo at the top of the page
}


//////////////////////////////////

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
      glColor3f(0, mSplashTimer.getFraction(), 1);

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
         renderBitfighterLogo(ctr, fr * 25.0f + 1, 1 << 8);
         renderBitfighterLogo(ctr, fr * 15.0f + 1, 1 << 9);
      }
   }
   else if(mPhase == 2)           // Resting phase
   {
      glColor3f(0, 0, 1);
      renderBitfighterLogo(gScreenInfo.getGameCanvasHeight() / 2, 1);
   }
   else if(mPhase == 3)           // Rising phase
   {
      glColor3f(0, sqrt(1 - mSplashTimer.getFraction()), 1 - pow(1 - mSplashTimer.getFraction(), 2));
      renderBitfighterLogo((S32)(73.0f + ((F32) gScreenInfo.getGameCanvasHeight() / 2.0f - 73.0f) * mSplashTimer.getFraction()), 1);
   }
}


void SplashUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();      //gMainMenuUserInterface
}


void SplashUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   quitting = true;
   quit();                              // Quit the interface when any key is pressed...  any key at all.

   if(keyCode != KEY_ESCAPE && keyCode != KEY_ENTER && keyCode != MOUSE_LEFT && keyCode != MOUSE_MIDDLE && keyCode != MOUSE_RIGHT)    // Unless user hit Enter or Escape, or some other thing
   {
      current->onKeyDown(keyCode, ascii);                // pass keystroke on  (after reactivate in quit(), current is now the underlying UI)
   }

   if(keyCode == MOUSE_LEFT && keyCode == MOUSE_MIDDLE && keyCode == MOUSE_RIGHT)
   {
      const Point *mousePos = gScreenInfo.getWindowMousePos();
      current->onMouseMoved((S32)mousePos->x, (S32)mousePos->y);
   }
}


};


