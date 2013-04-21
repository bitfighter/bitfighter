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
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _FONT_MANAGER_H_
#define _FONT_MANAGER_H_

#include "freeglut_stroke.h"     // Our stroke font handler -- include here to resolve namespace grief

extern "C" { 
#  include "../fontstash/fontstash.h" 
}

#include "tnlTypes.h"

#include <string>

using namespace TNL;
using namespace std;

struct sth_stash;

namespace Zap
{

class BfFont;

class FontManager 
{

private:
   static sth_stash *mStash;

   static S32 getStrokeFontStringLength(const SFG_StrokeFont *font, const char* string);
   static S32 getTtfFontStringLength(BfFont *font, const char* string);

public:
   enum FontId {
      FontRoman,
      FontOrbitronLightStroke,
      FontOrbitronMedStroke,
      FontOcrA,
      FontOrbitronLight,
      FontOrbitronMedium,
      FontPrimeRegular,
      FontTenby5,
      FontCount
   };

   enum FontContext {
      BigMessageContext,       // Press any key to respawn, etc.
      BubbleContext,           // Help bubbles
      LevelInfoContext,        // Display info about the level (at beginning of game, and when F2 pressed)
      MenuContext,             // Menu font (main game menus)
      HUDContext,              // General HUD text
      HelpContext,             // For Help screens
      LoadoutIndicatorContext, // For the obvious
      OverlayMenuContext,      // For Loadout Menus and such
      TextEffectContext        // Yard Sale!!! text and the like
   };

   static void initialize();    
   static void cleanup();   

   static sth_stash *getStash();

   static void drawTTFString(BfFont *font, const char *string, F32 size);
   static void drawStrokeCharacter(const SFG_StrokeFont *font, S32 character);

   static S32 getStringLength(const char* string);

   static void renderString(F32 size, const char *string);

   static void setFont(FontId fontId);
   static void setFontContext(FontContext fontContext);

   static void pushFontContext(FontContext fontContext);
   static void popFontContext();
};


////////////////////////////////////////
////////////////////////////////////////

class BfFont 
{
private:
   bool mIsStrokeFont;
   bool mOk;
   FontManager::FontId mFontId;

   S32 mStashFontId;

   const SFG_StrokeFont *mStrokeFont;     // Will be NULL for TTF fonts

public:
   BfFont(FontManager::FontId fontId, const ::SFG_StrokeFont *strokeFont);   // Stroke font constructor
   BfFont(FontManager::FontId fontId, const string &fontFile);               // TTF font constructor
   ~BfFont();                                                                // Destructor

   FontManager::FontId getId();
   const SFG_StrokeFont *getStrokeFont();
   bool isStrokeFont();
   S32 getStashFontId();

};


};

#endif

