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

#include "HelpBubble.h"
#include "UIGame.h"
#include "gameObjectRender.h"
#include "Colors.h"
#include "OpenglUtils.h"

using namespace TNL;

namespace Zap { namespace UI {


HelpBubble::HelpBubble(const Vector<string> &text, const AnchorPoint &anchor, GameUserInterface *parentUi)
{
   mText = text;
   mAnchor = anchor;
   mParentUi = parentUi;

   mFading = false;
   mDisplayTimer.reset(5000);
}


void HelpBubble::idle(U32 timeDelta)
{
   if(mDisplayTimer.update(timeDelta))
   {
      if(mFading)
         mParentUi->removeHelpBubble(this);
      else
      {
         mFading = true;
         mDisplayTimer.reset(500);
      }
   }
}


void HelpBubble::render()
{
   F32 alpha = 1;
   if(mFading)
      alpha = mDisplayTimer.getFraction();

   glColor(Colors::red, alpha);
   drawCircle(100,100,10);
}


} } // Nested namespace
