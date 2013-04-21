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
#include "FontManager.h"
#include "OpenglUtils.h"

using namespace TNL;

namespace Zap { namespace UI {


HelpBubble::HelpBubble(const Vector<string> &text, const AnchorPoint &anchor, GameUserInterface *parentUi)
{
   mText = text;
   mAnchor = anchor;
   mParentUi = parentUi;

   mWidth = calcWidth();
   mHeight = calcHeight();

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


static const S32 FontSize = 15;
static const S32 FontGap = 4;
static const S32 Margin = 5;           // Gap between edges and text

void HelpBubble::render()
{
   F32 alpha = 1;
   if(mFading)
      alpha = mDisplayTimer.getFraction();

   Point pos;
   if(mAnchor.anchorType == ScreenAnchor)    // Anchored to fixed location on the screen
      pos = mAnchor.pos;
   else                                      // Anchord to a fixed location on the map, may be offscreen
      pos = mAnchor.pos;      // ?????


   drawFilledRoundedRect(pos, mWidth, mHeight, Colors::red, Colors::white, 5, alpha);

   FontManager::pushFontContext(FontManager::BubbleContext);

   glColor(Colors::white, alpha);
   F32 yPos = pos.y - mHeight / 2 + FontSize + FontGap;

   for(S32 i = 0; i < mText.size(); i++)
   {
      drawStringc(pos.x, yPos, (F32)FontSize, mText[i].c_str());
      yPos += FontSize + FontGap;
   }

   FontManager::popFontContext();
}


S32 HelpBubble::calcWidth()
{
   S32 width = 0;

   for(S32 i = 0; i < mText.size(); i++)
   {
      S32 w = getStringWidth(FontManager::BubbleContext, FontSize, mText[i].c_str());
      width = max(width, w);
   }

   return width + 2 * Margin;
}


S32 HelpBubble::calcHeight()
{
   return mText.size() * (FontSize + FontGap) + 2 * Margin;
}


} } // Nested namespace
