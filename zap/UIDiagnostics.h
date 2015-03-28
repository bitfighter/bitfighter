//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UIDIAGNOSTIC_H_
#define _UIDIAGNOSTIC_H_

#include "UI.h"

namespace Zap
{


class FolderManager;

// Diagnostics UI
class DiagnosticUserInterface : public UserInterface
{
   typedef UserInterface Parent;
private:
   bool mActive;
   S32 mCurPage;

   static void initFoldersBlock(FolderManager *folderManager, S32 textsize);
   static S32 showFoldersBlock(FolderManager *folderManager, F32 textsize, S32 ypos, S32 gap);
   static S32 showVersionBlock(S32 ypos, S32 textsize, S32 gap);
   static S32 showNameDescrBlock(const string &hostName, const string &hostDescr, S32 ypos, S32 textsize, S32 gap);
   static S32 showMasterBlock(ClientGame *game, S32 textsize, S32 ypos, S32 gap, bool leftcol);


public:
   explicit DiagnosticUserInterface(ClientGame *game, UIManager *uiManager);     // Constructor
   virtual ~DiagnosticUserInterface();

   void onActivate();
   void idle(U32 t);
   void render() const;
   void quit();
   bool onKeyDown(InputCode inputCode);
   bool isActive() const;
};


}

#endif


