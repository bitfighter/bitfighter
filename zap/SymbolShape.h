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

#include "Joystick.h"      // For ButtonSymbol enum

#include "Color.h"
#include "Point.h"
#include "tnlVector.h"
#include "tnlTypes.h"

#include <boost/shared_ptr.hpp>


using namespace TNL;

namespace Zap { 
   
class InputCodeManager;

namespace UI {


enum Alignment {
   AlignmentLeft,
   AlignmentCenter,
   AlignmentRight,
   AlignmentNone     // Unspecified alignment
};


// Parent for various Shape classes below
class SymbolShape 
{
protected:
   S32 mWidth, mHeight;
   Point mLabelOffset;
   S32 mLabelSizeAdjustor;
   bool mHasColor;
   Color mColor;

public:
   SymbolShape(S32 width = 0, S32 height = 0, const Color *color = NULL);
   virtual ~SymbolShape();

   virtual void render(const Point &pos) const = 0;

   virtual S32 getWidth() const;
   virtual S32 getHeight() const;
   virtual bool getHasGap() const;  // Returns true if we automatically render a vertical blank space after this item
   virtual Point getLabelOffset(const string &label, S32 labelSize) const;
   virtual S32 getLabelSizeAdjustor(const string &label, S32 labelSize) const;
};


typedef boost::shared_ptr<SymbolShape> SymbolShapePtr;


class SymbolBlank : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolBlank(S32 width = -1, S32 height = -1);   // Constructor
   virtual ~SymbolBlank();

   void render(const Point &pos) const;
};


class SymbolHorizLine : public SymbolShape
{
   typedef SymbolShape Parent;

private:
   S32 mVertOffset;

public:
   SymbolHorizLine(S32 width, S32 height, const Color *color);                   // Constructor
   SymbolHorizLine(S32 width, S32 vertOffset, S32 height, const Color *color);   // Constructor
   virtual ~SymbolHorizLine();

   void render(const Point &pos) const;
};


class SymbolRoundedRect : public SymbolShape
{
   typedef SymbolShape Parent;

protected:
   S32 mRadius;

public:
   SymbolRoundedRect(S32 width, S32 height, S32 radius, const Color *color);   // Constructor
   virtual ~SymbolRoundedRect();

   virtual void render(const Point &pos) const;
};


// As above, but with slightly different rendering
class SymbolSmallRoundedRect : public SymbolRoundedRect
{
   typedef SymbolRoundedRect Parent;

public:
   SymbolSmallRoundedRect(S32 width, S32 height, S32 radius, const Color *color);   // Constructor
   virtual ~SymbolSmallRoundedRect();

   void render(const Point &pos) const;
};


class SymbolHorizEllipse : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolHorizEllipse(S32 width, S32 height, const Color *color); // Constructor
   virtual ~SymbolHorizEllipse();

   void render(const Point &pos) const;
};


class SymbolRightTriangle : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolRightTriangle(S32 width, const Color *color); // Constructor
   virtual ~SymbolRightTriangle();

   void render(const Point &pos) const;
};


class SymbolCircle : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolCircle(S32 radius, const Color *color); // Constructor
   virtual ~SymbolCircle();

   virtual void render(const Point &pos) const;
   S32 getLabelSizeAdjustor(const string &label, S32 labelSize) const;
   Point getLabelOffset(const string &label, S32 labelSize) const;
};


// Small glyphs for rendering on joystick buttons
class SymbolButtonSymbol : public SymbolShape
{
   typedef SymbolShape Parent;

private:
   Joystick::ButtonSymbol mGlyph;

public:
      SymbolButtonSymbol(Joystick::ButtonSymbol glyph);
      ~SymbolButtonSymbol();

      void render(const Point &pos) const;
};


class SymbolGear : public SymbolCircle
{
   typedef SymbolCircle Parent;

protected:
   F32 mSizeFactor;

public:
   SymbolGear(S32 fontSize);  // Constructor, fontSize is size of surrounding text
   virtual ~SymbolGear();

   virtual void render(const Point &pos) const;
};


class SymbolGoal : public SymbolGear
{
   typedef SymbolGear Parent;

public:
   SymbolGoal(S32 fontSize);  // Constructor, fontSize is size of surrounding text
   virtual ~SymbolGoal();

   void render(const Point &pos) const;
};


class SymbolText : public SymbolShape
{
   typedef SymbolShape Parent;

protected:
   string mText;
   FontContext mFontContext;
   S32 mFontSize;

public:
   SymbolText(const string &text, S32 fontSize, FontContext context, const Color *color = NULL);
   SymbolText(const string &text, S32 fontSize, FontContext context, const Point &labelOffset, const Color *color = NULL);
   virtual ~SymbolText();

   virtual void render(const Point &pos) const;
   S32 getHeight() const;

   bool getHasGap() const;
};


class SymbolKey : public SymbolText
{
   typedef SymbolText Parent;

public:
   SymbolKey(const string &text, const Color *color = NULL);
   virtual ~SymbolKey();

   void render(const Point &pos) const;
};


// Symbol to be used when we don't know what symbol to use
class SymbolUnknown : public SymbolKey
{
   typedef SymbolKey Parent;

public:
   SymbolUnknown(const Color *color);
   virtual ~SymbolUnknown();
};


////////////////////////////////////////
////////////////////////////////////////


class SymbolString : public SymbolShape      // So a symbol string can hold other symbol strings
{
   typedef SymbolShape Parent;

protected:
   S32 mWidth;
   S32 mReady;
   Alignment mAlignment;

   Vector<boost::shared_ptr<SymbolShape> > mSymbols;

public:
   SymbolString(const Vector<boost::shared_ptr<SymbolShape> > &symbols, Alignment alignment = AlignmentNone);
   SymbolString();                     // Constructor (can't use until you've setSymbols)
   virtual ~SymbolString();            // Destructor

   void setSymbols(const Vector<boost::shared_ptr<SymbolShape> > &symbols);

   // Dimensions
   virtual S32 getWidth() const;
   S32 getHeight() const;

   // Drawing
   S32 render(S32 x, S32 y, Alignment alignment, S32 blockWidth = -1) const;
   virtual S32 render(F32 x, F32 y, Alignment alignment, S32 blockWidth = -1) const;
   void render(const Point &center, Alignment alignment) const;
   void render(const Point &pos) const;

   bool getHasGap() const;

   // Statics to make creating things a bit easier
   static SymbolShapePtr getControlSymbol(InputCode inputCode, const Color *color = NULL);
   static SymbolShapePtr getModifiedKeySymbol(InputCode inputCode, const Vector<string> &modifiers, const Color *color = NULL);
   static SymbolShapePtr getSymbolGear(S32 fontSize);
   static SymbolShapePtr getSymbolGoal(S32 fontSize);
   static SymbolShapePtr getSymbolText(const string &text, S32 fontSize, FontContext context, const Color *color = NULL);
   static SymbolShapePtr getBlankSymbol(S32 width = -1, S32 height = -1);
   static SymbolShapePtr getHorizLine(S32 length, S32 height, const Color *color);
   static SymbolShapePtr getHorizLine(S32 length, S32 vertOffset, S32 height, const Color *color);

   //
   static void symbolParse(const InputCodeManager *inputCodeManager, const string &str, Vector<SymbolShapePtr> &symbols,
                           FontContext fontContext, S32 fontSize, const Color *color = NULL);

};


// As above, but all sumbols are layered atop one another, to create compound symbols like controller buttons
class LayeredSymbolString : public SymbolString
{
   typedef SymbolString Parent;

public:
   LayeredSymbolString(const Vector<boost::shared_ptr<SymbolShape> > &symbols);  // Constructor
   virtual ~LayeredSymbolString();                                               // Destructor

   S32 render(F32 x, F32 y, Alignment alignment, S32 blockWidth = -1) const;
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
   S32 getWidth() const;
   S32 getItemCount() const;
   S32 render(F32 x, F32 y, Alignment alignment, S32 blockWidth = -1) const;
   S32 render(S32 x, S32 y, Alignment alignment, S32 blockWidth = -1) const;
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
