//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _EDITOR_TEAM_H_
#define _EDITOR_TEAM_H_

#include "teamInfo.h"  // Parent class
#include "lineEditor.h"

#include <string>

namespace Zap
{


////////////////////////////////////////
////////////////////////////////////////

// Class for managing teams in the editor
class EditorTeam : public AbstractTeam
{
private:
   LineEditor mNameEditor;

public:
   EditorTeam();                          // Constructor
   explicit EditorTeam(const TeamPreset &preset);  // Constructor II
   virtual ~EditorTeam();                 // Destructor

   LineEditor *getLineEditor();
   void setName(const char *name);
   StringTableEntry getName() const;  // Wrap in STE to make signatures match

   S32 getPlayerBotCount() const;
   S32 getPlayerCount() const;    
   S32 getBotCount() const;
};

}

#endif
