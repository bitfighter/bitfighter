#include "SymbolShape.h"

#include "FontManager.h"

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
   drawString(center.x - mWidth / 2, center.y - fontSize / 2, fontSize, mText.c_str());
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
   mSymbols.deleteAndClear();    // Clean up those pointers
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



} } // Nested namespace


