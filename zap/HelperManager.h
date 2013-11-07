//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _HELPER_MANAGER_H_
#define _HELPER_MANAGER_H_

#include "ChatHelper.h"
#include "quickChatHelper.h"
#include "loadoutHelper.h"
#include "engineerHelper.h"
#include "TeamShuffleHelper.h"


namespace Zap
{


// Declare this here for conveinence... not really the ideal location
namespace UI 
{
   static const F32 DIM_LEVEL = .2f;
}


class HelperManager
{
private:
   // Various helper objects
   Vector<HelperMenu *> mHelperStack;        // Current helper
   HelperMenu *mOffDeckHelper;               // Kind of the opposite of on deck
   ClientGame *mGame;

   // Singleton helper classes
   ChatHelper        mChatHelper;
   QuickChatHelper   mQuickChatHelper;
   LoadoutHelper     mLoadoutHelper;
   EngineerHelper    mEngineerHelper;
   TeamShuffleHelper mTeamShuffleHelper;

   void doExitHelper(S32 index);

public:
   HelperManager();     // Constructor
   virtual ~HelperManager();

   void initialize(ClientGame *game);

   void onPlayerJoined();
   void onPlayerQuit();
   void onGameOver();
   void onTextInput(char ascii);

   void reset();

   void idle(U32 timeDelta);

   void render() const;
   F32 getFraction() const;

   void renderEngineeredItemDeploymentMarker(const Ship *ship);

   bool isChatAllowed() const;
   bool isMovementDisabled() const;
   bool isHelperActive() const;
   bool isHelperActive(HelperMenu::HelperMenuType helperType) const;

   bool processInputCode(InputCode inputCode);

   void activateHelp(UIManager *uiManager);              // Show the appropriate help screen, depending on which helper is active

   void quitEngineerHelper();
   void pregameSetup(bool isEnabled);                    // Set whether engineer is allowed on this level
   void setSelectedEngineeredObject(U32 objectType);

   void activateHelper(HelperMenu::HelperMenuType helperType, bool activatedWithChatCmd);
   void activateHelper(ChatHelper::ChatType chatType);   // Activate chat helper with specific chat subtype
   void exitHelper();
   void exitHelper(HelperMenu *helper);
   void doneClosingHelper();

   F32 getDimFactor() const;
   const char *getChatMessage() const;

   const HelperMenu *getActiveHelper() const;
};


}


#endif
