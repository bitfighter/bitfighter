//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_H_
#define _UI_H_

#ifdef ZAP_DEDICATED
#  error "UI.h shouldn't be included in dedicated build"
#endif

#include "lineEditor.h"
#include "InputCode.h"
#include "SymbolShape.h"

#include "Timer.h"
#include "tnl.h"
#include "tnlLog.h"

#include <string>

using namespace TNL;
using namespace std;

namespace Zap
{

using namespace UI;

extern F32 gLineWidth1;
extern F32 gDefaultLineWidth;
extern F32 gLineWidth3;
extern F32 gLineWidth4;

static const F32 HIGHLIGHTED_OBJECT_BUFFER_WIDTH = 14.0;      // Width to buffer objects higlighted by inline help system

////////////////////////////////////////
////////////////////////////////////////

class Game;
class ClientGame;
class GameSettings;
class UIManager;
class Color;

class UserInterface
{
   friend class UIManager;    // Give UIManager access to private and protected members

private:
   ClientGame *mClientGame;
   U32 mTimeSinceLastInput;

   static void doDrawAngleString(F32 x, F32 y, F32 size, F32 angle, const char *string, bool autoLineWidth = true);
   static void doDrawAngleString(S32 x, S32 y, F32 size, F32 angle, const char *string, bool autoLineWidth = true);

protected:
   bool mDisableShipKeyboardInput;                 // Disable ship movement while user is in menus

public:
   explicit UserInterface(ClientGame *game);       // Constructor
   virtual ~UserInterface();                       // Destructor

   static const S32 MOUSE_SCROLL_INTERVAL = 100;
   static const S32 MAX_PASSWORD_LENGTH = 32;      // Arbitrary, doesn't matter, but needs to be _something_

   static const S32 MaxServerNameLen = 40;
   static const S32 MaxServerDescrLen = 254;

   static const U32 StreakingThreshold = 5;        // This many kills in a row makes you a streaker!

   ClientGame *getGame() const;

   UIManager *getUIManager() const;

#ifdef TNL_OS_XBOX
   static const S32 horizMargin = 50;
   static const S32 vertMargin = 38;
#else
   static const S32 horizMargin = 15;
   static const S32 vertMargin = 15;
#endif

   static S32 messageMargin;

   U32 getTimeSinceLastInput();

   // Activate menus via the UIManager, please!
   void activate();
   void reactivate();

   virtual void render();
   virtual void idle(U32 timeDelta);
   virtual void onActivate();
   virtual void onDeactivate(bool nextUIUsesEditorScreenMode);
   virtual void onReactivate();
   virtual void onDisplayModeChange();

   virtual bool usesEditorScreenMode() const;   // Returns true if the UI attempts to use entire screen like editor, false otherwise

   void renderConsole()const;             // Render game console
   virtual void renderMasterStatus();     // Render master server connection status

   // Helpers to simplify dealing with key bindings
   static InputCode getInputCode(GameSettings *settings, InputCodeManager::BindingNameEnum binding);
   void setInputCode(GameSettings *settings, InputCodeManager::BindingNameEnum binding, InputCode inputCode);
   bool checkInputCode(InputCodeManager::BindingNameEnum, InputCode inputCode);
   const char *getInputCodeString(GameSettings *settings, InputCodeManager::BindingNameEnum binding);

   // Input event handlers
   virtual bool onKeyDown(InputCode inputCode);
   virtual void onKeyUp(InputCode inputCode);
   virtual void onTextInput(char ascii);
   virtual void onMouseMoved();
   virtual void onMouseDragged();

   // Old school -- deprecated
   void renderMessageBox(const string &title, const string &instr, const string &message, S32 vertOffset = 0, S32 style = 1) const;
   void renderMessageBox(const string &titleStr, const string &instrStr,
                         string *message, S32 msgLines, S32 vertOffset, S32 style) const;

   // New school
   void renderMessageBox(const SymbolShapePtr &title, const SymbolShapePtr &instr, SymbolShapePtr *message, S32 msgLines, S32 vertOffset = 0, S32 style = 1) const;

   static void renderCenteredFancyBox(S32 boxTop, S32 boxHeight, S32 inset, S32 cornerInset, const Color &fillColor, F32 fillAlpha, const Color &borderColor);

   static void dimUnderlyingUI(F32 amount = 0.75f);

   static void renderDiagnosticKeysOverlay();

   static void drawMenuItemHighlight(S32 x1, S32 y1, S32 x2, S32 y2, bool disabled = false);
   static void playBoop();    // Make some noise!
};


////////////////////////////////////////
////////////////////////////////////////

// Used only for multiple mClientGame in one instance
struct UserInterfaceData
{
   S32 vertMargin, horizMargin;
   S32 chatMargin;
};

};

#endif


