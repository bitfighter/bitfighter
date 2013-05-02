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

#ifndef _FONT_CONTEXT_ENUM_H_
#define _FONT_CONTEXT_ENUM_H_

namespace Zap
{     
   enum FontContext {
      BigMessageContext,       // Press any key to respawn, etc.
      HelpItemContext,         // In-game help messages
      LevelInfoContext,        // Display info about the level (at beginning of game, and when F2 pressed)
      MenuContext,             // Menu font (main game menus)
      HUDContext,              // General HUD text
      HelpContext,             // For Help screens
      LoadoutIndicatorContext, // For the obvious
      OverlayMenuContext,      // For Loadout Menus and such
      TextEffectContext        // Yard Sale!!! text and the like
   };


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

};


#endif
