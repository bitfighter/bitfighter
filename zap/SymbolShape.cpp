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
#include "Joystick.h"

#include "gameObjectRender.h"
#include "Colors.h"

#include "OpenglUtils.h"
#include "RenderUtils.h"

using namespace TNL;


namespace Zap { namespace UI {


SymbolStringSet::SymbolStringSet(S32 fontSize, S32 gap)
{
   mFontSize = fontSize;
   mGap = gap;
}


void SymbolStringSet::clear()
{
   mSymbolStrings.clear();
}


void SymbolStringSet::add(const SymbolString &symbolString)
{
   mSymbolStrings.push_back(symbolString);
}


void SymbolStringSet::renderLL(S32 x, S32 y) const
{
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
   {
      mSymbolStrings[i].renderLL(x, y);
      y += mFontSize + mGap;
   }
}


// x & y are coordinates of baseline of first item, will be centered in x direction
// Subsequent strings will be centered under the first
void SymbolStringSet::renderCL(S32 x, S32 y) const
{
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
   {
      mSymbolStrings[i].renderCC(x, y - mFontSize / 2);
      y += mFontSize + mGap;
   }
}


////////////////////////////////////////
////////////////////////////////////////

static S32 computeWidth(const Vector<SymbolShapePtr> &symbols, S32 fontSize, FontContext fontContext)
{
   S32 width = 0;

   for(S32 i = 0; i < symbols.size(); i++)
   {
      symbols[i]->updateWidth(fontSize, fontContext);
      width += symbols[i]->getWidth();
   }

   return width;
}


// Constructor with symbols
SymbolString::SymbolString(const Vector<SymbolShapePtr> &symbols, S32 fontSize, FontContext fontContext) : mSymbols(symbols)
{
   mFontSize    = fontSize;
   mFontContext = fontContext;
   mReady = true;

   mWidth = computeWidth(symbols, fontSize, fontContext);
}


// Constructor -- symbols will be provided later
SymbolString::SymbolString(S32 fontSize, FontContext fontContext)
{
   mFontSize    = fontSize;
   mFontContext = fontContext;
   mReady = false;

   mWidth = 0;
}


// Destructor
SymbolString::~SymbolString()
{
  // Do nothing
}


void SymbolString::setSymbols(const Vector<SymbolShapePtr> &symbols)
{
   mSymbols = symbols;

   mWidth = computeWidth(symbols, mFontSize, mFontContext);
   mReady = true;
}


S32 SymbolString::getWidth() const
{ 
   TNLAssert(mReady, "Not ready!");

   return mWidth;
}


// x & y are coordinates of lower left corner of where we want to render
void SymbolString::renderLL(S32 x, S32 y) const
{
   TNLAssert(mReady, "Not ready!");

   renderCC(Point(x + mWidth / 2, y - mFontSize / 2));
}


// Center is the point where we want the first string centered, vertically and horizontally
void SymbolString::renderCC(const Point &center) const
{
   renderCC((S32)center.x, (S32)center.y);
}


void SymbolString::renderCC(S32 x, S32 y) const
{
   TNLAssert(mReady, "Not ready!");

  x -= mWidth / 2;
  y += mFontSize / 2;

   FontManager::pushFontContext(mFontContext);

   for(S32 i = 0; i < mSymbols.size(); i++)
   {
      mSymbols[i]->render(Point(x + mSymbols[i]->getWidth() / 2, y), mFontSize, mFontContext);
      x += mSymbols[i]->getWidth();
   }

   FontManager::popFontContext();
}


static const S32 buttonHalfHeight = 9;   // This is the default half-height of a button
static const S32 rectButtonWidth = 24;
static const S32 rectButtonHeight = 18;
static const S32 smallRectButtonWidth = 19;
static const S32 smallRectButtonHeight = 15;
static const S32 horizEllipseButtonRadiusX = 14;
static const S32 horizEllipseButtonRadiusY = 8;
static const S32 rightTriangleWidth = 28;
static const S32 rightTriangleHeight = 18;
static const S32 RectRadius = 3;
static const S32 RoundedRectRadius = 5;


static SymbolShapePtr getSymbol(InputCode inputCode);

static SymbolShapePtr getSymbol(Joystick::ButtonShape shape)
{
   switch(shape)
   {
      case Joystick::ButtonShapeRound:
         return SymbolShapePtr(new SymbolCircle(buttonHalfHeight));

      case Joystick::ButtonShapeRect:
         return SymbolShapePtr(new SymbolRoundedRect(rectButtonWidth, 
                                                     rectButtonHeight, 
                                                     RectRadius));

      case Joystick::ButtonShapeSmallRect:
         return SymbolShapePtr(new SymbolRoundedRect(smallRectButtonWidth, 
                                                     smallRectButtonHeight, 
                                                     RectRadius));

      case Joystick::ButtonShapeRoundedRect:
         return SymbolShapePtr(new SymbolRoundedRect(rectButtonWidth, 
                                                     rectButtonHeight, 
                                                     RoundedRectRadius));

      case Joystick::ButtonShapeSmallRoundedRect:
         return SymbolShapePtr(new SymbolRoundedRect(smallRectButtonWidth, 
                                                     smallRectButtonHeight, 
                                                     RoundedRectRadius));
                                                     
      case Joystick::ButtonShapeHorizEllipse:
         return SymbolShapePtr(new SymbolHorizEllipse(horizEllipseButtonRadiusX, 
                                                      horizEllipseButtonRadiusY));

      case Joystick::ButtonShapeRightTriangle:
         return SymbolShapePtr(new SymbolRightTriangle(rightTriangleWidth));

      default:
         TNLAssert(false, "Unknown button shape!");
         return getSymbol(KEY_UNKNOWN);
   }
}


static SymbolShapePtr getSymbol(InputCode inputCode)
{
   if(InputCodeManager::isKeyboardKey(inputCode))
   {
      const char *str = InputCodeManager::inputCodeToString(inputCode);
      return SymbolShapePtr(new SymbolKey(str));
   }
   else if(inputCode == LEFT_JOYSTICK)
   {
      return getSymbol(Joystick::ButtonShapeRound);
   }
   else if(InputCodeManager::isControllerButton(inputCode))
   {
      // This gives us the logical button that inputCode represents... something like JoystickButton3
      JoystickButton button = InputCodeManager::inputCodeToJoystickButton(inputCode);

      // Now we need to figure out which symbol to use for this button, depending on controller make/model
      Joystick::ButtonInfo buttonInfo = Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].buttonMappings[button];

      // Don't render if button doesn't exist... what is this about???
      if(buttonInfo.sdlButton == Joystick::FakeRawButton)
         return getSymbol(KEY_UNKNOWN);

      // This gets us the button shape index, which will tell us what to draw... something like ButtonShapeRound
      Joystick::ButtonShape buttonShape = buttonInfo.buttonShape;

      SymbolShapePtr symbol = getSymbol(buttonShape);

      //const char *label = buttonInfo.label.c_str();
      //Color *buttonColor = &buttonInfo.color;

      return symbol;

   }
   else if(inputCode == KEY_UNKNOWN)
   {
      return SymbolShapePtr(new SymbolUnknown());
   }
   else
   {
      return getSymbol(KEY_UNKNOWN);
   }
}


// Static method
SymbolShapePtr SymbolString::getControlSymbol(InputCode inputCode)
{
   return getSymbol(inputCode);
}


// Static method
SymbolShapePtr SymbolString::getSymbolGear()
{
   return SymbolShapePtr(new SymbolGear());
}


////////////////////////////////////////
////////////////////////////////////////


// Destructor
SymbolShape::~SymbolShape()
{
   // Do nothing
}


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

// Destructor
SymbolRoundedRect::~SymbolRoundedRect()
{
   // Do nothing
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


// Destructor
SymbolHorizEllipse::~SymbolHorizEllipse()
{
   // Do nothing
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


// Destructor
SymbolRightTriangle::~SymbolRightTriangle()
{
   // Do nothing
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


// Destructor
SymbolCircle::~SymbolCircle()
{
   // Do nothing
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


// Destructor
SymbolGear::~SymbolGear()
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


// Destructor
SymbolText::~SymbolText()
{
   // Do nothing
}


void SymbolText::updateWidth(S32 fontSize, FontContext fontContext)
{
   mWidth = getStringWidth(fontContext, fontSize, mText.c_str());
}


void SymbolText::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   FontManager::pushFontContext(fontContext);
   drawStringc(center.x, center.y + fontSize / 2, (F32)fontSize, mText.c_str());
   FontManager::popFontContext();
}


////////////////////////////////////////
////////////////////////////////////////


SymbolKey::SymbolKey(const string &text) : Parent(text)
{
   // Do nothing
}


// Destructor
SymbolKey::~SymbolKey()
{
   // Do nothing
}


static S32 Margin = 3;              // Buffer within key around text
static S32 Gap = 3;                 // Distance between keys
static S32 FontSizeReduction = 4;   // How much smaller keycap font is than surrounding text
static S32 VertAdj = 2;             // To help with vertical centering

void SymbolKey::updateWidth(S32 fontSize, FontContext fontContext)
{
   fontSize -= FontSizeReduction;

   S32 width;
   
   if(mText == "Up Arrow" || mText == "Down Arrow" || mText == "Left Arrow" || mText == "Right Arrow")
      width = 0;     // Make a square button; mWidth will be set to mHeight below
   else
      width = getStringWidth(fontContext, fontSize, mText.c_str()) + Margin * 2;

   mHeight = fontSize + Margin * 2;
   mWidth = max(width, mHeight) + VertAdj * Gap;
}


void SymbolKey::render(const Point &center, S32 fontSize, FontContext fontContext) const
{
   static const Point vertAdj(0, VertAdj);

   fontSize -= FontSizeReduction;

   // Handle some special cases:
   if(mText == "Up Arrow")
      renderUpArrow(center + vertAdj, fontSize);
   else if(mText == "Down Arrow")
      renderDownArrow(center + vertAdj, fontSize);
   else if(mText == "Left Arrow")
      renderLeftArrow(center + vertAdj, fontSize);
   else if(mText == "Right Arrow")
      renderRightArrow(center + vertAdj, fontSize);
   else
      Parent::render(center + vertAdj, fontSize, KeyContext);

   S32 width =  max(mWidth - 2 * Gap, mHeight);

   drawHollowRect(center + vertAdj, width, mHeight);
}


////////////////////////////////////////
////////////////////////////////////////

// TODO: Override rendering with different color, if we add colors to this system

// Symbol to be used when we don't know what symbol to use

// Constructor
SymbolUnknown::SymbolUnknown() : Parent("~?~")
{
   // Do nothing
}


// Destructor
SymbolUnknown::~SymbolUnknown()
{
   // Do nothing
}


} } // Nested namespace


