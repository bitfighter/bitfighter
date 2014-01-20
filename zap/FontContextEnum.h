//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _FONT_CONTEXT_ENUM_H_
#define _FONT_CONTEXT_ENUM_H_

namespace Zap
{     
   enum FontContext {
      BigMessageContext,       // Press any key to respawn, etc.
      HelpItemContext,         // In-game help messages
      LevelInfoContext,        // Display info about the level (at beginning of game, and when F2 pressed)
      LevelInfoHeadlineContext,
      MenuContext,             // Menu font (main game menus)
      MenuHeaderContext,
      FPSContext,              // FPS display
      HelpContext,             // For Help screens
      ErrorMsgContext,         // For big red boxes and such
      KeyContext,              // For keyboard keys
      LoadoutIndicatorContext, // For the obvious
      HelperMenuContext,       // For Loadout Menus and such
      HelperMenuHeadlineContext,
      TextEffectContext,       // Yard Sale!!! text and the like
      ScoreboardContext,       // In-game scoreboard font
      ScoreboardHeadlineContext, // Headline items on scoreboard
      MotdContext,             // Scrolling MOTD on Main Menu
      InputContext,            // Input value font
      ReleaseVersionContext,   // Version number on title screen
      WebDingContext,          // Font for rendering database icon
      TeamShuffleContext,      // For /shuffle command
      ChatMessageContext,      // Font for rendering in-game chat messages
      OldSkoolContext,         // Render things like in the good ol' days
      TimeLeftHeadlineContext, // Big text on indicator in lower right corner of game  
      TimeLeftIndicatorContext // Smaller text on same indicator
   };


   // Please keep internal fonts separate at the top, away from the externally defined fonts
   enum FontId {
      // Internal fonts:
      FontRoman,                 // Our classic stroke font
      FontOrbitronLightStroke,
      FontOrbitronMedStroke,

      // External fonts:
      FontDroidSansMono,
      //FontOrbitronLight,
      //FontOrbitronMedium,
      FontWebDings,
      FontPlay,
      FontPlayBold,
      FontModernVision,

      FontCount,
      FirstExternalFont = FontDroidSansMono
   };

};


#endif
