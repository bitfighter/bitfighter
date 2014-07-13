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
   initialize();
}


// Constructor II
EditorTeam::EditorTeam(const TeamPreset &preset)
{
   initialize();

   setColor(preset.color);
   mNameEditor.setString(preset.name);
}


// Destructor
EditorTeam::~EditorTeam()
{
   // Do nothing
}


void EditorTeam::initialize()
{
   mNameEditor.mMaxLen = MAX_TEAM_NAME_LENGTH;
   mHexColorEditor.mMaxLen = 6;     // rrggbb
}


LineEditor *EditorTeam::getTeamNameEditor()
{
   return &mNameEditor;
}


LineEditor *EditorTeam::getHexColorEditor()
{
   return &mHexColorEditor;
}


void EditorTeam::setColor(F32 r, F32 g, F32 b)
{
   Parent::setColor(r, g, b);
   onColorChanged();
}


void EditorTeam::setColor(const Color &color)
{
   TeamInfo::setColor(color);    // TeamInfo is grandparent class
   onColorChanged();
}


void EditorTeam::setColor(const Color *color)
{
   TeamInfo::setColor(color);    // TeamInfo is grandparent class
   onColorChanged();
}


// Gets called when the color is changed
void EditorTeam::onColorChanged()
{
   mHexColorEditor.setString(getColor().toHexString());
}


void EditorTeam::setName(const char *name)
{
   mNameEditor.setString(name);
}


// Returns a STE to make signatures match
StringTableEntry EditorTeam::getName() const
{
   return StringTableEntry(mNameEditor.c_str());
}


};
