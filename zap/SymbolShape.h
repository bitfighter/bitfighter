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

#include "Color.h"
#include "Point.h"
#include "tnlVector.h"
#include "tnlTypes.h"

#include <boost/shared_ptr.hpp>


using namespace TNL;

namespace Zap { namespace UI {


enum Alignment {
   AlignmentLeft,
   AlignmentCenter,
   AlignmentRight
};


// Parent for various Shape classes below
class SymbolShape 
{
protected:
   S32 mWidth, mHeight;

public:
   SymbolShape(S32 width = 0, S32 height = 0);
   virtual ~SymbolShape();

   virtual void render(const Point &pos, S32 fontSize, FontContext fontContext) const = 0;
   virtual void updateWidth(S32 fontSize, FontContext fontContext);

   S32 getWidth() const;
   S32 getHeight() const;
   virtual bool getHasGap() const;  // Returns true if we automatically render a vertical blank space after this item
};


typedef boost::shared_ptr<SymbolShape> SymbolShapePtr;


class SymbolBlank : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolBlank(S32 width = -1, S32 height = -1);   // Constructor
   virtual ~SymbolBlank();

   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolHorizLine : public SymbolShape
{
   typedef SymbolShape Parent;

private:
   Color mColor;
   bool mUseColor;
   S32 mVertOffset;

public:
   SymbolHorizLine(S32 width, S32 height, const Color *color);                   // Constructor
   SymbolHorizLine(S32 width, S32 vertOffset, S32 height, const Color *color);   // Constructor
   virtual ~SymbolHorizLine();

   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
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
   FontContext mFontContext;
   S32 mFontSize;
   Color mColor;
   bool mUseColor;

public:
   SymbolText(const string &text, S32 fontSize, FontContext context, const Color *color = NULL);
   virtual ~SymbolText();

   virtual void updateWidth(S32 fontSize, FontContext fontContext);
   virtual void render(const Point &pos, S32 fontSize, FontContext fontContext) const;

   bool getHasGap() const;
};


class SymbolKey : public SymbolText
{
   typedef SymbolText Parent;

public:
   SymbolKey(const string &text, const Color *color = NULL);
   virtual ~SymbolKey();

   void updateWidth(S32 fontSize, FontContext fontContext);
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


//class SymbolModifiedKey : public SymbolKey
//{
//   typedef SymbolKey Parent;
//
//private:
//   string mBaseKey, mModifier;
//
//public:
//   SymbolModifiedKey(const string &text, const Color *color = NULL);
//   virtual ~SymbolModifiedKey();
//
//   void updateWidth(S32 fontSize, FontContext fontContext);
//   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
//};


// Symbol to be used when we don't know what symbol to use
class SymbolUnknown : public SymbolKey
{
   typedef SymbolKey Parent;

private:
   bool mUseColor;

public:
   SymbolUnknown(const Color *color);
   virtual ~SymbolUnknown();
};


////////////////////////////////////////
////////////////////////////////////////


class SymbolString : public SymbolShape      // So a symbol string can hold other symbol strings
{
private:
   S32 mWidth;
   S32 mFontSize;
   S32 mReady;

   FontContext mFontContext;

   Vector<boost::shared_ptr<SymbolShape> > mSymbols;

public:
   SymbolString(const Vector<boost::shared_ptr<SymbolShape> > &symbols, S32 fontSize, FontContext fontContext);   // Constructor
   SymbolString(S32 fontSize, FontContext fontContext);     // Constructor (can't use until you've setSymbols)
   virtual ~SymbolString();                                 // Destructor

   void setSymbols(const Vector<boost::shared_ptr<SymbolShape> > &symbols);

   // Dimensions
   S32 getWidth() const;
   S32 getHeight() const;

   // Drawing
   void render(S32 x, S32 y, Alignment alignment) const;
   void render(const Point &center, Alignment alignment) const;
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;

   bool getHasGap() const;

   static SymbolShapePtr getControlSymbol(InputCode inputCode, const Color *color = NULL);
   static SymbolShapePtr getSymbolGear();
   static SymbolShapePtr getSymbolText(const string &text, S32 fontSize, FontContext context, const Color *color = NULL);
   static SymbolShapePtr getBlankSymbol(S32 width = -1, S32 height = -1);
   static SymbolShapePtr getHorizLine(S32 length, S32 height, const Color *color);
   static SymbolShapePtr getHorizLine(S32 length, S32 vertOffset, S32 height, const Color *color);
};


class SymbolStringSet 
{
private:
   S32 mGap;

   Vector<SymbolString> mSymbolStrings;

public:
   SymbolStringSet(S32 gap);
   void clear();
   void add(const SymbolString &symbolString);
   S32 getHeight() const;
   S32 getItemCount() const;
   void render(S32 x, S32 y, Alignment alignment) const;
   S32 renderLine(S32 line, S32 x, S32 y, Alignment alignment) const;
};


////////////////////////////////////////
////////////////////////////////////////


class SymbolStringSetCollection
{
private:
   Vector<SymbolStringSet> mSymbolSet;
   Vector<Alignment> mAlignment;
   Vector<S32> mXPos;

public:
   void clear();
   void addSymbolStringSet(const SymbolStringSet &set, Alignment alignment, S32 xpos);
   S32 render(S32 yPos) const;
};



} } // Nested namespace



#endif
