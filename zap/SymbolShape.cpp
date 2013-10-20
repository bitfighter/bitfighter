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
#include "JoystickRender.h"

#include "gameObjectRender.h"
#include "Colors.h"

#include "OpenglUtils.h"
#include "RenderUtils.h"
#include "stringUtils.h"

using namespace TNL;


namespace Zap { namespace UI {


void SymbolStringSetCollection::clear()
{
   mSymbolSet.clear();
   mAlignment.clear();
   mXPos.clear();
}


void SymbolStringSetCollection::addSymbolStringSet(const SymbolStringSet &set, Alignment alignment, S32 xpos)
{
   mSymbolSet.push_back(set);
   mAlignment.push_back(alignment);
   mXPos.push_back(xpos);
}


S32 SymbolStringSetCollection::render(S32 yPos) const
{
   S32 lines = -1;

   // Figure out how many lines in our tallest SymbolStringSet
   for(S32 i = 0; i < mSymbolSet.size(); i++)
      lines = max(lines, mSymbolSet[i].getItemCount());

   // Render the SymbolStringSets line-by-line, keeping all lines aligned with one another.
   // Make a tally of the total height along the way (using the height of the tallest item rendered).
   S32 totalHeight = 0;

   for(S32 i = 0; i < lines; i++)
   {
      S32 height = 0;

      for(S32 j = 0; j < mSymbolSet.size(); j++)
      {
         S32 h = mSymbolSet[j].renderLine(i, mXPos[j], yPos + totalHeight, mAlignment[j]);
         height = max(h, height);   // Find tallest
      }

      totalHeight += height;
   }

   return totalHeight;
}


////////////////////////////////////////
////////////////////////////////////////


SymbolStringSet::SymbolStringSet(S32 gap)
{
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


S32 SymbolStringSet::getHeight() const
{
   S32 height = 0;
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
      height += mSymbolStrings[i].getHeight() + (mSymbolStrings[i].getHasGap() ? mGap : 0);

   return height;
}


S32 SymbolStringSet::getWidth() const
{
   S32 width = 0;
   for(S32 i = 0; i < mSymbolStrings.size(); i++)
      width = max(mSymbolStrings[i].getWidth(), width);

   return width;
}


S32 SymbolStringSet::getItemCount() const
{
   return mSymbolStrings.size();
}


S32 SymbolStringSet::render(S32 x, S32 y, Alignment alignment, S32 blockWidth) const
{
   return render(F32(x), F32(y), alignment, blockWidth);
}


S32 SymbolStringSet::render(F32 x, F32 yStart, Alignment alignment, S32 blockWidth) const
{
   S32 width = getWidth();
   S32 y = 0;

   for(S32 i = 0; i < mSymbolStrings.size(); i++)
   {
      mSymbolStrings[i].render(x, yStart + y, alignment, width);
      y += mSymbolStrings[i].getHeight() + mGap;
   }

   return y;
}


S32 SymbolStringSet::renderLine(S32 line, S32 x, S32 y, Alignment alignment) const
{
   // Make sure we're in bounds
   if(line >= mSymbolStrings.size())
      return 0;

   mSymbolStrings[line].render(x, y, alignment);
   return mSymbolStrings[line].getHeight() + (mSymbolStrings[line].getHasGap() ? mGap : 0);
}


////////////////////////////////////////
////////////////////////////////////////

// Width is the sum of the widths of all elements in the symbol list
static S32 computeWidth(const Vector<SymbolShapePtr > &symbols)
{
   S32 width = 0;

   for(S32 i = 0; i < symbols.size(); i++)
      width += symbols[i]->getWidth();

   return width;
}


// Width of a layered item is the widest of the widths of all elements in the symbol list
static S32 computeLayeredWidth(const Vector<SymbolShapePtr> &symbols)
{
   S32 width = 0;

   for(S32 i = 0; i < symbols.size(); i++)
   {
      S32 w = symbols[i]->getWidth();

      width = max(w, width);
   }

   return width;
}

// Height is the height of the tallest element in the symbol list
static S32 computeHeight(const Vector<SymbolShapePtr> &symbols)
{
   S32 height = 0;

   for(S32 i = 0; i < symbols.size(); i++)
   {
      S32 h = symbols[i]->getHeight();
      height = max(h, height);
   }

   return height;
}


// Constructor with symbols
SymbolString::SymbolString(const Vector<SymbolShapePtr> &symbols, Alignment alignment) : mSymbols(symbols)
{
   mReady = true;

   mWidth = computeWidth(symbols);
   mHeight = computeHeight(symbols);
   mAlignment = alignment;
}


SymbolString::SymbolString(const SymbolShapePtr &symbol, Alignment alignment)
{
   mSymbols.push_back(symbol);

   mReady = true;
   mWidth = symbol->getWidth();
   mHeight = symbol->getHeight();
   mAlignment = alignment;
}


// Constructor -- symbols will be provided later
SymbolString::SymbolString()
{
   mReady = false;

   mWidth = 0;
   mAlignment = AlignmentNone;
}


// Destructor
SymbolString::~SymbolString()
{
  // Do nothing
}


void SymbolString::setSymbols(const Vector<SymbolShapePtr> &symbols)
{
   mSymbols = symbols;

   mWidth = computeWidth(symbols);
   mReady = true;
}


S32 SymbolString::getWidth() const
{ 
   TNLAssert(mReady, "Not ready!");

   return mWidth;
}


S32 SymbolString::getHeight() const
{ 
   TNLAssert(mReady, "Not ready!");

   S32 height = -1;
   for(S32 i = 0; i < mSymbols.size(); i++)
      height = max(height, mSymbols[i]->getHeight());

   return height;
}


// Here to make class non-virtual
void SymbolString::render(const Point &pos) const
{
   render(pos, AlignmentCenter);
}


void SymbolString::render(const Point &center, Alignment alignment) const
{
   render((S32)center.x, (S32)center.y, alignment);
}


S32 SymbolString::render(S32 x, S32 y, Alignment blockAlignment, S32 blockWidth) const
{
   return render((F32)x, (F32)y, blockAlignment, blockWidth);
}


S32 SymbolString::render(F32 x, F32 y, Alignment blockAlignment, S32 blockWidth) const
{
   TNLAssert(mReady, "Not ready!");

   // Alignment of overall symbol string
   if(blockAlignment == AlignmentCenter)
      x -= mWidth / 2;     // x is now at the left edge of the render area

   if(blockWidth > -1)
   {
      // Individual line alignment
      Alignment lineAlignment;
      if(mAlignment == AlignmentNone)
         lineAlignment = blockAlignment;
      else
         lineAlignment = mAlignment;

      // TODO: Need to handle more cases here...
      if(lineAlignment == AlignmentLeft && blockAlignment == AlignmentCenter)
         x -= (blockWidth - mWidth) / 2;
   }

   for(S32 i = 0; i < mSymbols.size(); i++)
   {
      S32 w = mSymbols[i]->getWidth();
      mSymbols[i]->render(Point(x + w / 2, y));
      x += w;
   }

   return mHeight;
}


bool SymbolString::getHasGap() const
{
   for(S32 i = 0; i < mSymbols.size(); i++)
      if(mSymbols[i]->getHasGap())
         return true;

   return false;
}


static const S32 buttonHalfHeight = 9;   // This is the default half-height of a button
static const S32 rectButtonWidth = 24;
static const S32 rectButtonHeight = 18;
static const S32 smallRectButtonWidth = 19;
static const S32 smallRectButtonHeight = 15;
static const S32 horizEllipseButtonDiameterX = 28;
static const S32 horizEllipseButtonDiameterY = 16;
static const S32 rightTriangleWidth = 28;
static const S32 rightTriangleHeight = 18;
static const S32 RectRadius = 3;
static const S32 RoundedRectRadius = 5;


static SymbolShapePtr getSymbol(InputCode inputCode, const Color *color);

static SymbolShapePtr getSymbol(Joystick::ButtonShape shape, const Color *color)
{
   switch(shape)
   {
      case Joystick::ButtonShapeRound:
         return SymbolShapePtr(new SymbolCircle(buttonHalfHeight, color));

      case Joystick::ButtonShapeRect:
         return SymbolShapePtr(new SymbolRoundedRect(rectButtonWidth, 
                                                     rectButtonHeight, 
                                                     RectRadius,
                                                     color));

      case Joystick::ButtonShapeSmallRect:
         return SymbolShapePtr(new SymbolSmallRoundedRect(smallRectButtonWidth, 
                                                          smallRectButtonHeight, 
                                                          RectRadius,
                                                          color));

      case Joystick::ButtonShapeRoundedRect:
         return SymbolShapePtr(new SymbolRoundedRect(rectButtonWidth, 
                                                     rectButtonHeight, 
                                                     RoundedRectRadius,
                                                     color));

      case Joystick::ButtonShapeSmallRoundedRect:
         return SymbolShapePtr(new SymbolSmallRoundedRect(smallRectButtonWidth, 
                                                          smallRectButtonHeight, 
                                                          RoundedRectRadius,
                                                          color));
                                                     
      case Joystick::ButtonShapeHorizEllipse:
         return SymbolShapePtr(new SymbolHorizEllipse(horizEllipseButtonDiameterX, 
                                                      horizEllipseButtonDiameterY,
                                                      color));

      case Joystick::ButtonShapeRightTriangle:
         return SymbolShapePtr(new SymbolRightTriangle(rightTriangleWidth, color));

      default:
         //TNLAssert(false, "Unknown button shape!");
         return getSymbol(KEY_UNKNOWN, &Colors::red);
   }
}


static SymbolShapePtr getSymbol(Joystick::ButtonShape shape, const string &label, const Color *color)
{
   static const S32 LabelSize = 13;
   Vector<SymbolShapePtr> symbols;
   
   // Get the button outline
   SymbolShapePtr shapePtr = getSymbol(shape, color);

   symbols.push_back(shapePtr);

   // Handle some special cases -- there are some button labels that refer to special glyphs
   Joystick::ButtonSymbol buttonSymbol = Joystick::stringToButtonSymbol(label);

   if(buttonSymbol == Joystick::ButtonSymbolNone)
      symbols.push_back(SymbolShapePtr(new SymbolText(label, LabelSize + shapePtr->getLabelSizeAdjustor(label, LabelSize), 
                                                      KeyContext, shapePtr->getLabelOffset(label, LabelSize))));
   else
      symbols.push_back(SymbolShapePtr(new SymbolButtonSymbol(buttonSymbol)));

   return SymbolShapePtr(new LayeredSymbolString(symbols));
}


static S32 KeyFontSize = 13;     // Size of characters used for rendering key bindings

// Color is ignored for controller buttons
static SymbolShapePtr getSymbol(InputCode inputCode, const Color *color)
{
   if(InputCodeManager::isKeyboardKey(inputCode))
      return SymbolShapePtr(new SymbolKey(InputCodeManager::inputCodeToString(inputCode), color));

   // These items are rendered as just plain old text... for no particular reason, but it seems to work at the moment
   else if(inputCode == LEFT_JOYSTICK)
      return SymbolString::getSymbolText("Left Joystick", KeyFontSize, KeyContext, color);
   else if(inputCode == RIGHT_JOYSTICK)
      return SymbolString::getSymbolText("Right Joystick", KeyFontSize, KeyContext, color);
   else if(inputCode == MOUSE)
      return SymbolString::getSymbolText("Mouse", KeyFontSize, KeyContext, color);

   // The following items are rendered like keys, with boxes around the text -- they are often mixed with other keys
   // and look weird if they are just bare text
   else if(inputCode == MOUSE_LEFT)
      return SymbolShapePtr(new SymbolKey("Left Mouse Button", color));
   else if(inputCode == MOUSE_MIDDLE)
      return SymbolShapePtr(new SymbolKey("Middle Mouse Button", color));
   else if(inputCode == MOUSE_RIGHT)
      return SymbolShapePtr(new SymbolKey("Right Mouse Button", color));
   else if(inputCode == MOUSE_WHEEL_UP)
      return SymbolShapePtr(new SymbolKey("Mouse Wheel Up", color));
   else if(inputCode == MOUSE_WHEEL_DOWN)
      return SymbolShapePtr(new SymbolKey("Mouse Wheel Down", color));
   else if(InputCodeManager::isCtrlKey(inputCode) || InputCodeManager::isAltKey(inputCode))
   {
      Vector<string> modifiers(1);
      if(InputCodeManager::isCtrlKey(inputCode))
         modifiers.push_back(InputCodeManager::inputCodeToString(KEY_CTRL));
      else // if(isAltKey(inputCode))
         modifiers.push_back(InputCodeManager::inputCodeToString(KEY_ALT));
      
      return SymbolString::getModifiedKeySymbol(InputCodeManager::getBaseKey(inputCode), modifiers, color);
   }
   else if(InputCodeManager::isControllerButton(inputCode))
   {
      // This gives us the logical button that inputCode represents... something like JoystickButton3
      JoystickButton button = InputCodeManager::inputCodeToJoystickButton(inputCode);

      // Now we need to figure out which symbol to use for this button, depending on controller make/model
      Joystick::ButtonInfo buttonInfo = Joystick::JoystickPresetList[Joystick::SelectedPresetIndex].buttonMappings[button];

      if(!Joystick::isButtonDefined(Joystick::SelectedPresetIndex, button))
         return getSymbol(KEY_UNKNOWN, color);

      // This gets us the button shape index, which will tell us what to draw... something like ButtonShapeRound
      Joystick::ButtonShape buttonShape = buttonInfo.buttonShape;

      SymbolShapePtr symbol = getSymbol(buttonShape, buttonInfo.label, &buttonInfo.color);

      return symbol;

   }
   else if(inputCode == KEY_UNKNOWN)
      return SymbolShapePtr(new SymbolUnknown(color));

   else
      return getSymbol(KEY_UNKNOWN, color);
}


SymbolShapePtr SymbolString::getModifiedKeySymbol(const string &symbolName, const Color *color)
{
   const Vector<string> *mods = InputCodeManager::getModifierNames();
   Vector<string> foundMods;

   bool found = true;
   string sym = symbolName;      // Make working copy

   while(found)
   {
      for(S32 i = 0; i < mods->size(); i++)
      {
         string mod = mods->get(i) + "+";
         found = false;
         if(sym.compare(0, mod.size(), mod) == 0)     // Read as: sym.startsWith(mod)
         {
            foundMods.push_back(mods->get(i));
            sym = sym.substr(mod.size());
            found = true;
            break;
         }
      }
   }

   InputCode inputCode = InputCodeManager::stringToInputCode(sym.c_str());     // Get the base inputCode

   if(inputCode == KEY_UNKNOWN) 
       return SymbolShapePtr();

   SymbolShapePtr s = SymbolString::getModifiedKeySymbol(inputCode, foundMods, color);
   return s;
}


SymbolShapePtr SymbolString::getModifiedKeySymbol(InputCode inputCode, const Vector<string> &modifiers, const Color *color)
{
   if(inputCode == KEY_UNKNOWN || modifiers.size() == 0)
      return getSymbol(inputCode, color);

   Vector<SymbolShapePtr> symbols;
   for(S32 i = 0; i < modifiers.size(); i++)
   {
      symbols.push_back(SymbolShapePtr(new SymbolKey(modifiers[i], color)));
      symbols.push_back(SymbolShapePtr(new SymbolText("+", 13, KeyContext, Point(0, -3), color))); // Use offset to vertically center "+"
   }

   symbols.push_back(SymbolShapePtr(new SymbolKey(InputCodeManager::inputCodeToString(inputCode), color)));

   SymbolShape *s = new SymbolString(symbols);

   return SymbolShapePtr(s);
}


// Static method
SymbolShapePtr SymbolString::getControlSymbol(InputCode inputCode, const Color *color)
{
   return getSymbol(inputCode, color);
}


// Static method
SymbolShapePtr SymbolString::getSymbolGear(S32 fontSize)
{
   return SymbolShapePtr(new SymbolGear(fontSize));
}


// Static method
SymbolShapePtr SymbolString::getSymbolGoal(S32 fontSize)
{
   return SymbolShapePtr(new SymbolGoal(fontSize));
}


// Static method
SymbolShapePtr SymbolString::getSymbolNexus(S32 fontSize)
{
   return SymbolShapePtr(new SymbolNexus(fontSize));
}


// Static method
SymbolShapePtr SymbolString::getSymbolSpinner(S32 fontSize, const Color *color)
{
   return SymbolShapePtr(new SymbolSpinner(fontSize, color));
}


// Static method
SymbolShapePtr SymbolString::getBullet()
{
   return SymbolShapePtr(new SymbolBullet());
}


// Static method
SymbolShapePtr SymbolString::getSymbolText(const string &text, S32 fontSize, FontContext context, const Color *color)
{
   return SymbolShapePtr(new SymbolText(text, fontSize, context, color));
}


// Static method
SymbolShapePtr SymbolString::getBlankSymbol(S32 width, S32 height)
{
   return SymbolShapePtr(new SymbolBlank(width, height));
}


// Static method
SymbolShapePtr SymbolString::getHorizLine(S32 length, S32 height, const Color *color)
{
   return SymbolShapePtr(new SymbolHorizLine(length, height, color));
}


SymbolShapePtr SymbolString::getHorizLine(S32 length, S32 vertOffset, S32 height, const Color *color)
{
   return SymbolShapePtr(new SymbolHorizLine(length, vertOffset, height, color));
}


// Parse special symbols enclosed inside [[ ]] in strings.  The passed symbolName is the bit inside the brackets.
static void getSymbolShape(const InputCodeManager *inputCodeManager, const string &symbol, 
                           const Color *color, Vector<SymbolShapePtr> &symbols)
{
   Vector<string> words;
   parseString(symbol, words, ':');

   string symbolName = words[0];

   // The following will return KEY_UNKNOWN if symbolName is not recognized as a known binding
   InputCode inputCode = inputCodeManager->getKeyBoundToBindingCodeName(symbolName);
   
   // Second chance -- maybe it's a key name instead of a control binding (like "K")
   if(inputCode == KEY_UNKNOWN)
      inputCode = inputCodeManager->stringToInputCode(symbolName.c_str());

   // Third chance -- see if there is a modifier key at the front (like "Ctrl+K")
   if(inputCode == KEY_UNKNOWN)
   {
      SymbolShapePtr modifiedKey = SymbolString::getModifiedKeySymbol(symbolName, color);
      if(modifiedKey.get() != NULL)
      {
         symbols.push_back(modifiedKey);
         return;
      }
   }

   if(inputCode != KEY_UNKNOWN)
      symbols.push_back(SymbolString::getControlSymbol(inputCode, color));
   else if(symbolName == "LOADOUT_ICON")
      symbols.push_back(SymbolString::getSymbolGear(14));
   else if(symbolName == "GOAL_ICON")
      symbols.push_back(SymbolString::getSymbolGoal(14));
   else if(symbolName == "NEXUS_ICON")
      symbols.push_back(SymbolString::getSymbolNexus(14));
   else if(symbolName == "SPINNER")
      symbols.push_back(SymbolString::getSymbolSpinner(14, color));
   else if(symbolName == "CHANGEWEP")
   {
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP1)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP2)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_SELWEAP3)));
   }

   else if(symbolName == "MOVEMENT")
   {
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_UP)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_DOWN)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_LEFT)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_RIGHT)));
   }

   else if(symbolName == "MOVEMENT_LDR")
   {
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_LEFT)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_DOWN)));
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_RIGHT)));
   }

   else if(symbolName == "MODULE_CTRL1")
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_MOD1)));

   else if(symbolName == "MODULE_CTRL2")
      symbols.push_back(SymbolString::getControlSymbol(inputCodeManager->getBinding(InputCodeManager::BINDING_MOD2)));

   else if(symbolName == "BULLET")                    // Square bullet point 
      symbols.push_back(SymbolString::getBullet());

   else if(symbolName == "TAB_STOP")                  // Adds whitespace until width is equal to n
   {
      TNLAssert(words.size() == 2, "TAB_STOP:n has the wrong number of components!");
      S32 width = atoi(words[1].c_str());

      S32 w = 0;
      for(S32 i = 0; i < symbols.size(); i++)
         w += symbols[i]->getWidth();

      symbols.push_back(SymbolShapePtr(new SymbolBlank(width - w)));
   }

   else 
      symbols.push_back(SymbolShapePtr(new SymbolText("Unknown Symbol: " + symbolName, 12, HelpItemContext, &Colors::red)));
}


void SymbolString::symbolParse(const InputCodeManager *inputCodeManager, const string &str, Vector<SymbolShapePtr> &symbols,
                              FontContext fontContext, S32 fontSize, const Color *textColor, const Color *symbolColor)
{
   std::size_t offset = 0;

   if(!symbolColor)
         symbolColor = textColor;

   while(true)
   {
      std::size_t startPos = str.find("[[", offset);      // If this isn't here, no further searching is necessary
      std::size_t endPos   = str.find("]]", offset + 2);

      if(startPos == string::npos || endPos == string::npos)
      {
         // No further symbols herein, convert the rest to text symbol and exit
         symbols.push_back(SymbolShapePtr(new SymbolText(str.substr(offset), fontSize, fontContext, textColor)));
         return;
      }

      symbols.push_back(SymbolShapePtr(new SymbolText(str.substr(offset, startPos - offset), fontSize, fontContext, textColor)));

      // Use + 2 to advance past the opening "[["
      getSymbolShape(inputCodeManager, str.substr(startPos + 2, endPos - startPos - 2), symbolColor, symbols); 

      offset = endPos + 2;
   }
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
LayeredSymbolString::LayeredSymbolString(const Vector<boost::shared_ptr<SymbolShape> > &symbols) :
                  Parent(symbols)
{
   mWidth = computeLayeredWidth(symbols);
}


// Destructor
LayeredSymbolString::~LayeredSymbolString()
{
   // Do nothing
}


// Each layer is rendered atop the previous, creating a layered effect
S32 LayeredSymbolString::render(F32 x, F32 y, Alignment alignment, S32 blockWidth) const
{
   TNLAssert(mReady, "Not ready!");

   for(S32 i = 0; i < mSymbols.size(); i++)
      mSymbols[i]->render(Point(x, y));

   return mHeight;
}


////////////////////////////////////////
////////////////////////////////////////

SymbolShape::SymbolShape(S32 width, S32 height, const Color *color) : mColor(color)
{
   mWidth = width;
   mHeight = height;
   mHasColor = color != NULL;
   mLabelSizeAdjustor = 0;
}


// Destructor
SymbolShape::~SymbolShape()
{
   // Do nothing
}


void SymbolShape::render(F32 x, F32 y) const
{
   render(Point(x,y));
}


void SymbolShape::render(S32 x, S32 y, Alignment alignment) const
{
   render((F32)x, (F32)y, alignment);
}


void SymbolShape::render(F32 x, F32 y, Alignment alignment) const
{
   if(alignment == AlignmentLeft)
      x += mWidth / 2;

   render(Point(x, y));
}


S32 SymbolShape::getWidth() const
{
   return mWidth;
}


S32 SymbolShape::getHeight() const
{
   return mHeight;
}


bool SymbolShape::getHasGap() const
{
   return false;
}


Point SymbolShape::getLabelOffset(const string &label, S32 labelSize) const
{
   return mLabelOffset;
}


S32 SymbolShape::getLabelSizeAdjustor(const string &label, S32 labelSize) const
{
   return mLabelSizeAdjustor;
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolBlank::SymbolBlank(S32 width, S32 height) : Parent(width, height, NULL)
{
   // Do nothing
}


// Destructor
SymbolBlank::~SymbolBlank()
{
   // Do nothing
}


void SymbolBlank::render(const Point &center) const
{
   // Do nothing -- it's blank, remember?
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolHorizLine::SymbolHorizLine(S32 length, S32 height, const Color *color) : Parent(length, height, color)
{
   mVertOffset = 0;
}


// Constructor
SymbolHorizLine::SymbolHorizLine(S32 length, S32 vertOffset, S32 height, const Color *color) : Parent(length, height, color)
{
   mVertOffset = vertOffset;
}


// Destructor
SymbolHorizLine::~SymbolHorizLine()
{
   // Do nothing
}


void SymbolHorizLine::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   drawHorizLine(center.x - mWidth / 2, center.x + mWidth / 2, center.y - mHeight / 2 + mVertOffset);
}


////////////////////////////////////////
////////////////////////////////////////

static const S32 BorderDecorationVertCenteringOffset = 2;   // Offset the border of keys and buttons to better center them in the flow of text
static const S32 SpacingAdjustor = 2;


// Constructor
SymbolRoundedRect::SymbolRoundedRect(S32 width, S32 height, S32 radius, const Color *color) : 
                                                                  Parent(width + SpacingAdjustor, height + SpacingAdjustor, color)
{
   mRadius = radius;
}


// Destructor
SymbolRoundedRect::~SymbolRoundedRect()
{
   // Do nothing
}


void SymbolRoundedRect::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   drawRoundedRect(center - Point(0, (mHeight - SpacingAdjustor) / 2 - BorderDecorationVertCenteringOffset - 1), 
                   mWidth - SpacingAdjustor, mHeight - SpacingAdjustor, mRadius);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolSmallRoundedRect::SymbolSmallRoundedRect(S32 width, S32 height, S32 radius, const Color *color) : 
                                                               Parent(width + SpacingAdjustor, height + SpacingAdjustor, radius, color)
{
   mLabelOffset.set(0, -1);
}


// Destructor
SymbolSmallRoundedRect::~SymbolSmallRoundedRect()
{
   // Do nothing
}


void SymbolSmallRoundedRect::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   drawRoundedRect(center - Point(0, mHeight / 2 - BorderDecorationVertCenteringOffset - SpacingAdjustor + 2), 
                   mWidth - SpacingAdjustor, mHeight - SpacingAdjustor, mRadius);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolHorizEllipse::SymbolHorizEllipse(S32 width, S32 height, const Color *color) : Parent(width + 2, height, color)
{
   mLabelOffset.set(0, -1);
}


// Destructor
SymbolHorizEllipse::~SymbolHorizEllipse()
{
   // Do nothing
}


void SymbolHorizEllipse::render(const Point &center) const
{
   S32 w = mWidth / 2;
   S32 h = mHeight / 2;

   if(mHasColor)
      glColor(mColor);

   Point cen = center - Point(0, h - 1);

   // First the fill
   drawFilledEllipse(cen, w, h, 0);

   // Outline in white
   glColor(Colors::white);
   drawEllipse(cen, w, h, 0);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolRightTriangle::SymbolRightTriangle(S32 width, const Color *color) : Parent(width, 19, color)
{
   mLabelOffset.set(-5, -1);
   mLabelSizeAdjustor = -3;
}


// Destructor
SymbolRightTriangle::~SymbolRightTriangle()
{
   // Do nothing
}


static void drawButtonRightTriangle(const Point &center)
{
   Point p1(center + Point(-6, -15));
   Point p2(center + Point(-6,   4));
   Point p3(center + Point(21,  -6));

   F32 vertices[] = {
         p1.x, p1.y,
         p2.x, p2.y,
         p3.x, p3.y
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINE_LOOP);
}


void SymbolRightTriangle::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   Point cen(center.x -mWidth / 4, center.y);  // Need to off-center the label slightly for this button
   drawButtonRightTriangle(cen);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolCircle::SymbolCircle(S32 radius, const Color *color) : Parent(radius * 2 + SpacingAdjustor, radius * 2 + SpacingAdjustor, color)
{
   // Do nothing
}


// Destructor
SymbolCircle::~SymbolCircle()
{
   // Do nothing
}


void SymbolCircle::render(const Point &pos) const
{
   if(mHasColor)
      glColor(mColor);

   // Adjust our position's y coordinate to be the center of the circle
   drawCircle(pos - Point(0, (mHeight) / 2 - BorderDecorationVertCenteringOffset - SpacingAdjustor), F32(mWidth - SpacingAdjustor) / 2);
}


static const S32 LabelAutoShrinkThreshold = 15;

S32 SymbolCircle::getLabelSizeAdjustor(const string &label, S32 labelSize) const
{
   // Shrink labels a little when the text is uncomfortably big for the button
   if(getStringWidth(labelSize, label.c_str()) > LabelAutoShrinkThreshold)
      return mLabelSizeAdjustor - 2;
   
   return mLabelSizeAdjustor;
}


Point SymbolCircle::getLabelOffset(const string &label, S32 labelSize) const
{
   if(getStringWidth(labelSize, label.c_str()) > LabelAutoShrinkThreshold)
      return mLabelOffset + Point(0, -1);

   return mLabelOffset;
}


////////////////////////////////////////
////////////////////////////////////////


SymbolButtonSymbol::SymbolButtonSymbol(Joystick::ButtonSymbol glyph)
{
   mGlyph = glyph;
}


SymbolButtonSymbol::~SymbolButtonSymbol()
{
   // Do nothing
}


void SymbolButtonSymbol::render(const Point &pos) const
{
   // Get symbol in the proper position for rendering -- it's either this or change all the render methods
   Point renderPos = pos + Point(0, -6);   

   switch(mGlyph)
   {
      case Joystick::ButtonSymbolPsCircle:
         JoystickRender::drawPlaystationCircle(renderPos);
         break;
      case Joystick::ButtonSymbolPsCross:
         JoystickRender::drawPlaystationCross(renderPos);
         break;
      case Joystick::ButtonSymbolPsSquare:
         JoystickRender::drawPlaystationSquare(renderPos);
         break;
      case Joystick::ButtonSymbolPsTriangle:
         JoystickRender::drawPlaystationTriangle(renderPos);
         break;
      case Joystick::ButtonSymbolSmallLeftTriangle:
         JoystickRender::drawSmallLeftTriangle(renderPos + Point(0, -1));
         break;
      case Joystick::ButtonSymbolSmallRightTriangle:
         JoystickRender::drawSmallRightTriangle(renderPos + Point(0, -1));
         break;
      case Joystick::ButtonSymbolNone:
      default:
         TNLAssert(false, "Shouldn't be here!");
         break;
   }
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor
SymbolGear::SymbolGear(S32 fontSize) : Parent(0, NULL)
{
   mWidth = S32(1.333f * fontSize);    // mWidth is effectively a diameter; we'll use mWidth / 2 for our rendering radius
   mHeight = mWidth;
}


// Destructor
SymbolGear::~SymbolGear()
{
   // Do nothing
}


void SymbolGear::render(const Point &pos) const
{
   // We are given the bottom y position of the line, but the icon expects the center
   Point center(pos.x, (pos.y - mHeight/2) + 2); // Slight downward adjustment to position to better align with text
   renderLoadoutZoneIcon(center, mWidth / 2);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolGoal::SymbolGoal(S32 fontSize) : Parent(fontSize)
{
   // Do nothing
}


// Destructor
SymbolGoal::~SymbolGoal()
{
   // Do nothing
}


void SymbolGoal::render(const Point &pos) const
{
   // We are given the bottom y position of the line, but the icon expects the center
   Point center(pos.x, (pos.y - mHeight/2) + 2); // Slight downward adjustment to position to better align with text
   renderGoalZoneIcon(center, mWidth/2);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolNexus::SymbolNexus(S32 fontSize) : Parent(fontSize)
{
   // Do nothing
}


// Destructor
SymbolNexus::~SymbolNexus()
{
   // Do nothing
}


void SymbolNexus::render(const Point &pos) const
{
   // We are given the bottom y position of the line, but the icon expects the center
   Point center(pos.x, (pos.y - mHeight/2) + 2); // Slight downward adjustment to position to better align with text
   renderNexusIcon(center, mWidth/2);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SymbolSpinner::SymbolSpinner(S32 fontSize, const Color *color) : Parent(fontSize / 2, color)
{
   // Do nothing
}


// Destructor
SymbolSpinner::~SymbolSpinner()
{
   // Do nothing
}


void SymbolSpinner::render(const Point &pos) const
{
   S32 charindx = Platform::getRealMilliseconds() / 200 % 4;

   const char *charstr;

   switch(charindx)
   {
      case 0:
         charstr = "|";
         break;
      case 1:
         charstr = "/";
         break;
      case 2:
         charstr = "--";
         break;
      case 3:
         charstr = "\\";
         break;
      default: 
         TNLAssert(false, "Unexpected value of charindx");
         break;
   }

   drawStringc(pos, mHeight, charstr);
}


////////////////////////////////////////
////////////////////////////////////////

static const S32 BulletRad = 2;

// Constructor
SymbolBullet::SymbolBullet() : Parent(BulletRad * 2, BulletRad * 2)
{
   // Do nothing
}


// Destructor
SymbolBullet::~SymbolBullet()
{
   // Do nothing
}


void SymbolBullet::render(const Point &pos) const
{
   // We are given the bottom y position of the line, but the icon expects the center
   Point center(pos.x, (pos.y - 7));
   drawFilledSquare(center, BulletRad);
}


////////////////////////////////////////
////////////////////////////////////////


// Constructor with no vertical offset
SymbolText::SymbolText(const string &text, S32 fontSize, FontContext context, const Color *color) : 
                              Parent(getStringWidth(context, fontSize, text.c_str()), fontSize, color)
{
   mText = text;
   mFontContext = context;
   mFontSize = fontSize;
}


// Constructor with vertical offset
SymbolText::SymbolText(const string &text, S32 fontSize, FontContext context, const Point &labelOffset, const Color *color) : 
                                       Parent(getStringWidth(context, fontSize, text.c_str()), fontSize, color)
{
   mText = text;
   mFontContext = context;
   mFontSize = fontSize;
   mLabelOffset = labelOffset;

   mHeight = fontSize;
}


// Destructor
SymbolText::~SymbolText()
{
   // Do nothing
}


void SymbolText::render(const Point &center) const
{
   if(mHasColor)
      glColor(mColor);

   FontManager::pushFontContext(mFontContext);
   drawStringc(center + mLabelOffset, (F32)mFontSize, mText.c_str());
   FontManager::popFontContext();
}


S32 SymbolText::getHeight() const
{
   return Parent::getHeight() + (S32)mLabelOffset.y;
}


bool SymbolText::getHasGap() const
{
   return true;
}


////////////////////////////////////////
////////////////////////////////////////


static S32 Margin = 3;              // Buffer within key around text
static S32 Gap = 3;                 // Distance between keys
static S32 TotalHeight = KeyFontSize + 2 * Margin;
static S32 SymbolPadding = 6;       // Just some padding we throw around our symbols to make them look hot


static S32 getKeyWidth(const string &text, S32 height)
{
   S32 width = -1;
   if(text == "Up Arrow" || text == "Down Arrow" || text == "Left Arrow" || text == "Right Arrow")
      width = 0;     // Make a square button; will return height below (and since it's a square...)
   else
      width = getStringWidth(KeyContext, KeyFontSize, text.c_str()) + Margin * 2;

   return max(width, height) + SymbolPadding;
}


SymbolKey::SymbolKey(const string &text, const Color *color) : Parent(text, KeyFontSize, KeyContext, color)
{
   mHeight = TotalHeight;
   mWidth = getKeyWidth(text, mHeight);
}


// Destructor
SymbolKey::~SymbolKey()
{
   // Do nothing
}


// Note: passed font size and context will be ignored
void SymbolKey::render(const Point &center) const
{
   // Compensate for the fact that boxes draw from center
   const Point boxVertAdj  = mLabelOffset + Point(0, BorderDecorationVertCenteringOffset - KeyFontSize / 2 - 3);   
   const Point textVertAdj = mLabelOffset + Point(0, BorderDecorationVertCenteringOffset - 3);

   if(mHasColor)
      glColor(mColor);

   // Handle some special cases:
   if(mText == "Up Arrow")
      renderUpArrow(center + textVertAdj + Point(0, -5.5), KeyFontSize);
   else if(mText == "Down Arrow")
      renderDownArrow(center + textVertAdj + Point(0, -6), KeyFontSize);
   else if(mText == "Left Arrow")
      renderLeftArrow(center + textVertAdj + Point(0, -6), KeyFontSize);
   else if(mText == "Right Arrow")
      renderRightArrow(center + textVertAdj + Point(0, -6), KeyFontSize);
   else
      Parent::render(center + textVertAdj);

   S32 width =  max(mWidth - 2 * Gap, mHeight);

   drawHollowRect(center + boxVertAdj, width, mHeight);
}


////////////////////////////////////////
////////////////////////////////////////


// Symbol to be used when we don't know what symbol to use

// Constructor
SymbolUnknown::SymbolUnknown(const Color *color) : Parent("~?~", &Colors::red)
{
   // Do nothing
}


// Destructor
SymbolUnknown::~SymbolUnknown()
{
   // Do nothing
}


} } // Nested namespace


