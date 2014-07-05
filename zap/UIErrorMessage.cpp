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

   InputCodeManager *inputCodeManager = getGame()->getSettings()->getInputCodeManager();

   for(S32 i = 0; i < wrappedLines.size(); i++)
      mMessage[i] = SymbolShapePtr(new SymbolString(wrappedLines[i], inputCodeManager, Context, TextHeight, true));

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
   mTitle = SymbolShapePtr(new SymbolString(title, getGame()->getSettings()->getInputCodeManager(), Context, TitleSize, false));
}


void AbstractMessageUserInterface::setInstr(const string &instr)
{
   mInstr = SymbolShapePtr(new SymbolString(instr, getGame()->getSettings()->getInputCodeManager(), Context, TextHeight, false));
}


void AbstractMessageUserInterface::quit()
{
   getUIManager()->reactivatePrevUI();
}


void AbstractMessageUserInterface::reset()
{
   for(S32 i = 0; i < MAX_LINES; i++)
      mMessage[i] = SymbolShapePtr(new SymbolBlank());

   mMaxLines = MAX_LINES;
   mKeyRegistrations.clear();
   mRenderUnderlyingUi = true;

   mInstr = SymbolShapePtr(new SymbolBlank());
   mTitle = SymbolShapePtr(new SymbolBlank());
}


void AbstractMessageUserInterface::registerKey(InputCode key, void(*callback)(ClientGame *))
{
   mKeyRegistrations[key] = callback;
}


void AbstractMessageUserInterface::setRenderUnderlyingUi(bool render)
{
   mRenderUnderlyingUi = render;
}


bool AbstractMessageUserInterface::onKeyDown(InputCode inputCode)
{
   bool handled = Parent::onKeyDown(inputCode);

   if(handled)
      return true;

   if(inputCode == KEY_ESCAPE)
   {
      quit();
      return true;
   }


   if(mKeyRegistrations.find(inputCode) != mKeyRegistrations.end())
   {
      mKeyRegistrations[inputCode](getGame());
      return true;
   }

   return false;
}


void AbstractMessageUserInterface::render() const
{
   if(mRenderUnderlyingUi && getUIManager()->getPrevUI() != this)
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


bool ErrorMessageUserInterface::usesEditorScreenMode() const
{
   if(getUIManager()->getCurrentUI() == this)
   {
      TNLAssert(getUIManager()->getPrevUI() != this, "Why same UI twice?");
      return getUIManager()->getPrevUI()->usesEditorScreenMode();
   }
   else
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


