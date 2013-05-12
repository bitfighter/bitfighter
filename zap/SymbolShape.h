
#ifndef _SYMBOL_SHAPE_H_
#define _SYMBOL_SHAPE_H_

#include "FontContextEnum.h"
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
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolHorizEllipse : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolHorizEllipse(S32 width, S32 height); // Constructor
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolRightTriangle : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolRightTriangle(S32 width); // Constructor
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolCircle : public SymbolShape
{
   typedef SymbolShape Parent;

public:
   SymbolCircle(S32 radius); // Constructor
   virtual void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolGear : public SymbolCircle
{
   typedef SymbolCircle Parent;

private:
   F32 mSizeFactor;

public:
   SymbolGear(); // Constructor
   void updateWidth(S32 fontSize, FontContext fontContext);
   void render(const Point &pos, S32 fontSize, FontContext fontContext) const;
};


class SymbolText : public SymbolShape
{
   typedef SymbolShape Parent;

private:
   string mText;

public:
   SymbolText(const string &text);
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
   FontContext mFontContext;

   Vector<SymbolShape *> mSymbols;

public:
   SymbolString(const Vector<SymbolShape *> &symbols, S32 fontSize, FontContext fontContext);      // Constructor
   virtual SymbolString::~SymbolString();

   S32 getWidth() const;
   void renderCenter(const Point &center) const;
};


} } // Nested namespace



#endif