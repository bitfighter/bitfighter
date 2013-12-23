//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "EditorTeam.h"


namespace Zap
{


// Constructor
EditorTeam::EditorTeam()
{
   mNameEditor.mMaxLen = MAX_TEAM_NAME_LENGTH;
}


// Constructor II
EditorTeam::EditorTeam(const TeamPreset &preset)
{
   mNameEditor.mMaxLen = MAX_TEAM_NAME_LENGTH;
   mNameEditor.setString(preset.name);
   mColor = Color(preset.r, preset.g, preset.b);
}


// Destructor
EditorTeam::~EditorTeam()
{
   // Do nothing
}


LineEditor *EditorTeam::getLineEditor()
{
   return &mNameEditor;
}


void EditorTeam::setName(const char *name)
{
   mNameEditor.setString(name);
}


StringTableEntry EditorTeam::getName() const
{
   return StringTableEntry(mNameEditor.c_str());
}


S32 EditorTeam::getPlayerBotCount() const
{
   return 0;
}


S32 EditorTeam::getPlayerCount() const
{
   return 0;
}


S32 EditorTeam::getBotCount() const
{
   return 0;
}   



};
