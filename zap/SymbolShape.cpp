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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "SymbolShape.h"

#include "FontManager.h"
#include "InputCode.h"

#include "gameObjectRender.h"
#include "Colors.h"

#include "OpenglUtils.h"
#include "RenderUtils.h"

using namespace TNL;


namespace Zap { namespace UI {


S32 SymbolShape::getWidth() const
{
   return mWidth;
}


void SymbolShape::updateWidth(S32 fontSize, FontContext fontContext)
{
   // Do nothing (is overridden)
}



// Constructor
SymbolRoundedRect::SymbolRoundedRect(S32 width, S32 height, S32 radius)
{
   mWidth = width;
   mHeight = height;
   mRadius = radius;
}


void SymbolRoundedRect::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   drawRoundedRect(center, mWidth, mHeight, mRadius);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolHorizEllipse::SymbolHorizEllipse(S32 width, S32 height)
{
   mWidth = width;
   mHeight = height;
}


void SymbolHorizEllipse::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   // First the fill
   drawFilledEllipse(center, mWidth, mHeight, 0);

   // Outline in white
   glColor(Colors::white);
   drawEllipse(center, mWidth, mHeight, 0);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolRightTriangle::SymbolRightTriangle(S32 width)
{
   mWidth = width;
}


static void drawButtonRightTriangle(const Point &center)
{
   Point p1(center + Point(-15, -9));
   Point p2(center + Point(-15, 10));
   Point p3(center + Point(12, 0));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void SymbolRightTriangle::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   Point cen(center.x -mWidth / 4, center.y);  // Need to off-center the label slightly for this button
   drawButtonRightTriangle(cen);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolCircle::SymbolCircle(S32 radius)
{
   mWidth = radius * 2;
   mHeight = radius * 2;
}

void SymbolCircle::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   drawCircle(center, (F32)mWidth / 2);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolGear::SymbolGear() : Parent(0)
{
   // Do nothing
}


void SymbolGear::updateWidth(S32 fontSize, FontContext fontContext)
{
   mWidth = S32(1.333f * fontSize);    // mWidth is effectively a diameter; we'll use mWidth / 2 for our rendering radius
   mHeight = mWidth;
}


void SymbolGear::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   renderLoadoutZoneIcon(center + Point(0,2), mWidth / 2);    // Slight downward adjustment to position to better align with text
}


////////////////////////////////////////
////////////////////////////////////////


SymbolText::SymbolText(const string &text)
{
   mText = text;
   mWidth = -1;
}


void SymbolText::updateWidth(S32 fontSize, FontContext fontContext)
{
   mWidth = getStringWidth(fontContext, fontSize, mText.c_str());
}


void SymbolText::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   FontManager::pushFontContext(fontContext);
   drawString(center.x - mWidth / 2, center.y - fontSize / 2, fontSize, mText.c_str());
   FontManager::popFontContext();
}


////////////////////////////////////////
////////////////////////////////////////


SymbolKey::SymbolKey(const string &text) : Parent(text)
{
   // Do nothing
}


void SymbolKey::updateWidth(S32 fontSize, FontContext fontContext)
{
   mWidth = getStringWidth(KeyContext, fontSize, mText.c_str());
}


void SymbolKey::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   Parent::render(center, fontSize, KeyContext);
}


////////////////////////////////////////
////////////////////////////////////////


SymbolString::SymbolString(const Vector<SymbolShape *> &symbols, S32 fontSize, FontContext fontContext)
{
   mSymbols     = symbols;
   mFontSize    = fontSize;
   mFontContext = fontContext;

   mWidth = 0;

   for(S32 i = 0; i < mSymbols.size(); i++)
   {
      mSymbols[i]->updateWidth(fontSize, fontContext);
      mWidth += mSymbols[i]->getWidth();
   }
}


SymbolString::~SymbolString()
{
   mSymbols.clear();    // Clean up those pointers
}


S32 SymbolString::getWidth() const
{ 
   return mWidth;
}


void SymbolString::renderCenter(const Point &center) const
{
   S32 x = (S32)center.x - getWidth() / 2;
   S32 y = (S32)center.y;

   FontManager::pushFontContext(mFontContext);

   for(S32 i = 0; i < mSymbols.size(); i++)
   {
      mSymbols[i]->render(Point(x + mSymbols[i]->getWidth() / 2, y + mFontSize / 2), mFontSize, mFontContext);
      x += mSymbols[i]->getWidth();
   }

   FontManager::popFontContext();
}


// Locally defined class, used only here, and only for ensuring objects are cleaned up on exit
class SymbolHolder
{
private:
   static Vector<SymbolShape *> mKeySymbols;
   static SymbolGear *mSymbolGear;

public:
   SymbolHolder() 
   {
      if(mKeySymbols.size() == 0)                     // Should always be the case
         mKeySymbols.resize(LAST_KEYBOARD_KEY + 1);   // Values will be initialized to NULL
   }


   ~SymbolHolder()
   {
      mKeySymbols.deleteAndClear();                   // Delete objects created in getSymbol
      delete mSymbolGear;
   }


   SymbolShape *getSymbol(InputCode inputCode)
   {
      // Lazily initialize -- we're unlikely to actually need more than a few of these during a session
      if(!mKeySymbols[inputCode])
      {
         string stuff = InputCodeManager::inputCodeToGlyph(inputCode);
         if(stuff != "")
            mKeySymbols[inputCode] = new SymbolKey(stuff);
         else
            mKeySymbols[inputCode] = new SymbolText("[" + string(InputCodeManager::inputCodeToString(inputCode)) + "]");
      }

      return mKeySymbols[inputCode];
   }


   SymbolShape *getSymbolGear()
   {
      if(!mSymbolGear)
         mSymbolGear = new SymbolGear();

      return mSymbolGear;
   }

};

// Statics used by SymbolHolder
Vector<SymbolShape *> SymbolHolder::mKeySymbols;      
SymbolGear *SymbolHolder::mSymbolGear = NULL;

static SymbolHolder symbolHolder;


// Static method
SymbolShape *SymbolString::getControlSymbol(InputCode inputCode)
{
   if(InputCodeManager::isKeyboardKey(inputCode))
      return symbolHolder.getSymbol(inputCode);
   else
   { 
      TNLAssert(false, "Deal with it!");
      return NULL;      // Certain to crash
   }
}


SymbolShape *SymbolString::getSymbolGear()
{
   return symbolHolder.getSymbolGear();
}


} } // Nested namespace


