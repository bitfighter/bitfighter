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
   typedef AbstractTeam Parent;

private:
   LineEditor mNameEditor;
   LineEditor mHexColorEditor;

   void initialize();

public:
   EditorTeam();                                   // Constructor
   explicit EditorTeam(const TeamPreset &preset);  // Constructor II
   virtual ~EditorTeam();                          // Destructor

   LineEditor *getTeamNameEditor();
   LineEditor *getHexColorEditor();

   void setName(const char *name);
   StringTableEntry getName() const;  // Returns a STE to make signatures match

   void setColor(F32 r, F32 g, F32 b);
   void setColor(const Color &color);
};

}

#endif
