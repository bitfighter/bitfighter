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

#ifndef _SYMBOL_SHAPE_H_
#define _SYMBOL_SHAPE_H_

#include "FontContextEnum.h"
#include "InputCodeEnum.h"

#include "Point.h"
#include "tnlVector.h"
#include "tnlTypes.h"


using namespace TNL;

namespace Zap { namespace UI {


// Parent for various Shape classes below
class SymbolShape 
{
protected:
   S32 mWidth, mHeight;
public:
   virtual ~SymbolShape();

   virtual void render(const Point &pos, S32 fontSize, FontContext fontContext) const = 0;
   virtual void updateWidth(S32 fontSize, FontContext fontContext);

   S32 getWidth() const;
};


class SymbolRoundedRect : public SymbolShape
{
   typedef SymbolShape Parent;

private:
   S32 mRadius;

public:
   SymbolRoundedRect(S32 width, S32 height, S32 radius);   // Constructor
   virtual ~SymbolRoundedRect();

   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolHorizEllipse : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolHorizEllipse(S32 width, S32 height); // Constructor
   virtual ~SymbolHorizEllipse();

   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolRightTriangle : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolRightTriangle(S32 width); // Constructor
   virtual ~SymbolRightTriangle();

   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolCircle : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolCircle(S32 radius); // Constructor
   virtual ~SymbolCircle();

   virtual void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolGear : public SymbolCircle
{
   typedef SymbolCircle Parent;

private:
   F32 mSizeFactor;

public:
   SymbolGear();  // Constructor
   virtual ~SymbolGear();

   void updateWidth(S32 fontSize, FontContext fontContext);
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolText : public SymbolShape
{
   typedef SymbolShape Parent;

protected:
   string mText;

public:
   SymbolText(const string &text);
   virtual ~SymbolText();

   virtual void updateWidth(S32 fontSize, FontContext fontContext);
   virtual void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolKey : public SymbolText
{
   typedef SymbolText Parent;

public:
   SymbolKey(const string &text);
   virtual ~SymbolKey();

   void updateWidth(S32 fontSize, FontContext fontContext);
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


////////////////////////////////////////
////////////////////////////////////////


class SymbolString
{
private:
   S32 mWidth;
   S32 mFontSize;
   S32 mReady;

   FontContext mFontContext;

   Vector<SymbolShape *> mSymbols;

public:
   SymbolString(const Vector<SymbolShape *> &symbols, S32 fontSize, FontContext fontContext);   // Constructor
   SymbolString(S32 fontSize, FontContext fontContext);     // Constructor (can't use until you've setSymbols)
   virtual SymbolString::~SymbolString();                   // Destructor

   void setSymbols(const Vector<SymbolShape *> &symbols);

   S32 getWidth() const;

   void renderLL(S32 x, S32 y) const;
   void renderCC(const Point &center) const;

   static SymbolShape *getControlSymbol(InputCode inputCode);

   static SymbolShape *getSymbolGear();
};


class SymbolStringSet 
{
private:
   S32 mFontSize;
   S32 mGap;

   Vector<SymbolString> mSymbolStrings;

public:
   SymbolStringSet(S32 fontSize, S32 gap);
   void clear();
   void add(const SymbolString &symbolString);
   void renderLL(S32 x, S32 y) const;
};

} } // Nested namespace



#endif