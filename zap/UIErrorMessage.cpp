//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "UIErrorMessage.h"

#include "UIManager.h"
#include "ClientGame.h"

#include "RenderUtils.h"

namespace Zap
{

// Constructor
AbstractMessageUserInterface::AbstractMessageUserInterface(ClientGame *game) : Parent(game)
{
   reset();
}


AbstractMessageUserInterface::~AbstractMessageUserInterface()
{
   // Do nothing
}


void AbstractMessageUserInterface::onActivate()
{
   // Do nothing -- block parent's onActivate
}


// TODO: These are repeated in UI.cpp... bad!
static const S32 TextHeight = 18;
static const S32 TitleSize = 30;

static const FontContext Context = ErrorMsgContext;

void AbstractMessageUserInterface::setMessage(const string &message)
{
   Vector<string> wrappedLines;
   wrapString(message, UIManager::MessageBoxWrapWidth, TextHeight, Context, wrappedLines);

   Vector<UI::SymbolShapePtr> symbols;

   for(S32 i = 0; i < wrappedLines.size(); i++)
   {
      symbols.clear();
      SymbolString::symbolParse(getGame()->getSettings()->getInputCodeManager(), wrappedLines[i],
                                symbols, Context, TextHeight);

      mMessage[i] = SymbolShapePtr(new SymbolString(symbols));
   }

   mMaxLines = wrappedLines.size();
}


// Use this to limit the size of the box
void AbstractMessageUserInterface::setMaxLines(S32 lines)
{
   TNLAssert(lines <= MAX_LINES, "Invalid value for lines!");
   mMaxLines = lines - 1;
}


void AbstractMessageUserInterface::setTitle(const string &title)
{
   Vector<UI::SymbolShapePtr> symbols;

   SymbolString::symbolParse(getGame()->getSettings()->getInputCodeManager(), title, symbols, Context, TitleSize);

   mTitle = SymbolShapePtr(new SymbolString(symbols));
}


void AbstractMessageUserInterface::setInstr(const string &instr)
{
   Vector<UI::SymbolShapePtr> symbols;

   SymbolString::symbolParse(getGame()->getSettings()->getInputCodeManager(), instr, symbols, Context, TextHeight);

   mInstr = SymbolShapePtr(new SymbolString(symbols));
}


void AbstractMessageUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();
}


void AbstractMessageUserInterface::reset()
{
   for(S32 i = 0; i < MAX_LINES; i++)
      mMessage[i] =  SymbolShapePtr(new SymbolBlank());

   mMaxLines = MAX_LINES;
}


bool AbstractMessageUserInterface::onKeyDown(InputCode inputCode)
{
   return Parent::onKeyDown(inputCode);
}


void AbstractMessageUserInterface::render()
{
   if(getUIManager()->getPrevUI() != this)
      getUIManager()->renderPrevUI(this);

   renderMessageBox(mTitle, mInstr, mMessage, mMaxLines);
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
ErrorMessageUserInterface::ErrorMessageUserInterface(ClientGame *game) : Parent(game)
{
   // Do nothing
}


// Destructor
ErrorMessageUserInterface::~ErrorMessageUserInterface()
{
   // Do nothing
}


bool ErrorMessageUserInterface::usesEditorScreenMode()
{
   return getUIManager()->getCurrentUI()->usesEditorScreenMode();
}


// Return true if key handled
bool ErrorMessageUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode))
      return true;
   else if(inputCode == KEY_ESCAPE)
   {
      quit();     // Quit the interface when any key is pressed...  any key at all.  Mostly.  Well, if that key was Escape.
      return true;
   }

   return false;
}


};


