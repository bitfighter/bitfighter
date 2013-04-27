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

#ifndef _ENGINEERHELPER_H_
#define _ENGINEERHELPER_H_

#include "helperMenu.h"
#include "Engineerable.h"     // For EngineerBuildObjects enum

using namespace TNL;

namespace Zap
{

class Ship;

class EngineerHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   const char *getCancelMessage() const;
   S32 mSelectedIndex;

   bool isMenuBeingDisplayed() const;
   void exitHelper();

public:
   explicit EngineerHelper();    // Constructor
   virtual ~EngineerHelper();    // Destructor

   HelperMenuType getType();

   void setSelectedEngineeredObject(U32 objectType);

   void onActivated();
   bool processInputCode(InputCode inputCode);   
   void render();                
   void renderDeploymentMarker(const Ship *ship);

   bool isChatDisabled();
   S32 getAnimationTime() const;
};

};

#endif


