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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _CONFIG_H_
#define _CONFIG_H_

// This file contains definitions of two structs that are used to store our
// INI settings and command line param settings, which are read separately,
// but processed jointly.  Some default values are provided here as well,
// especially for the INI settings...  if the INI is deleted, these defaults
// will be used to rebuild it.

#include "../tnl/tnlTypes.h"
#include "../tnl/tnlNetStringTable.h"

#include "input.h"     
#include <string>

using namespace std;
using namespace Zap;


namespace Zap
{

extern string gLevelDir;

struct CmdLineSettings
{
   bool clientMode;        // Start with client active
   bool serverMode;        // Start in server mode
   bool connectRemote;

   string server;
   string connect;         // Connect to this server immediately
   string masterAddress;   // Use this master server
   F32 loss;               // Simulate packet loss (0-1)
   U32 lag;                // Simulate server lag (in ms)
   string dedicated;
   string name;
   string defaultName;     // Name used if user hits <enter> on name entry screen
   string lastName;        // Name the user previously entered

   string hostname;
   string hostaddr;        // Address to listen on when we're host (e.g. IP:localhost:1234 or IP:Any:6666 or whatever)
   string hostdescr;       // One-line description of server
   string password;
   string adminPassword;
   string levelChangePassword;
   string levelDir;        // Subfolder under levels specified with -leveldir parameter
   S32 maxplayers;

   bool window;            // Window param supplied
   bool fullscreen;        // Fullscreen param supplied

   S32 winWidth;
   S32 xpos;
   S32 ypos;

   bool suppliedLevels;    // Did the user specify any levels on the cmd line?

   void init()
   {
      clientMode = true;
      serverMode = false;
      connectRemote = false;

      server = "";
      connect = "";
      masterAddress = "";
      defaultName = "";
      lastName = "";
      hostaddr = "";
      loss = 0;
      lag = 0;
      dedicated = "";
      name = "";
      password = "";
      adminPassword = "";
      levelChangePassword = "";
      hostname = "";
      hostdescr = "";
      levelDir = "";
      maxplayers = -1;
      window = false;
      fullscreen = false;
      suppliedLevels = false;
      winWidth = -1;
      xpos = -9999;
      ypos = -9999;
   };
};

enum sfxSets {
   sfxClassicSet,
   sfxModernSet
};


struct IniSettings      // With defaults specified
{
   bool controlsRelative;
   bool fullscreen;
   S32 joystickType;
   bool echoVoice;

   F32 sfxVolLevel;                 // SFX volume (0 = silent, 1 = full bore)
   F32 musicVolLevel;               // As above
   F32 voiceChatVolLevel;           // Ditto
   F32 alertsVolLevel;              // And again

   sfxSets sfxSet;                  // Which set of SFX does the user want?

   bool starsInDistance;            // True if stars move in distance behind maze, false if they are in fixed location
   bool diagnosticKeyDumpMode;      // True if want to dump keystrokes to the screen

   bool showWeaponIndicators;       // True if we show the weapon indicators on the top of the screen
   bool verboseHelpMessages;        // If true, we'll show more handholding messages
   bool showKeyboardKeys;           // True if we show the keyboard shortcuts in joystick mode

   bool enableExperimentalAimMode;  // True if we want to show an aim vector in joystick mode

   InputMode inputMode;             // Joystick or Keyboard
   string masterAddress;            // Default address of our master server
   string name;                     // Player name (none by default)
   string defaultName;              // Name used if user hits <enter> on name entry screen
   string lastName;                 // Name user entered last time the game was run -- will be used as default on name entry screen

   string hostname;                 // Server name when in host mode
   string hostaddr;                 // User-specified address/port of server
   string hostdescr;                // One-line description of server
   string password;
   string adminPassword;
   string levelChangePassword;      // Password to allow access to level changing functionality on non-local server
   string levelDir;                 // Folder where levels are stored, by default
   S32 maxplayers;                  // Max number of players that can play on local server

   // Game window location when in windowed mode
   S32 winXPos;
   S32 winYPos;
   F32 winSizeFact;

   // Testing values
   S32 burstGraphicsMode;           // Choose a burst graphic representation
   S32 szGraphicsMode;              // Choose a speedzone graphic

   Vector<StringTableEntry> levelList;
 
   Vector<string> reservedNames;
   Vector<string> reservedPWs;


   // Set default values here
   void init()
   {
      controlsRelative = false;          // Relative controls is lame!
      fullscreen = true;
      joystickType = NoController;
      echoVoice = false;

      sfxVolLevel = 1.0;                 // SFX volume (0 = silent, 1 = full bore)
      musicVolLevel = 1.0;               // Music volume (range as above)
      voiceChatVolLevel = 1.0;           // INcoming voice chat volume (range as above)
      alertsVolLevel = 1.0;              // Audio alerts volume (when in dedicated server mode only, range as above)

      sfxSet = sfxModernSet;             // Start off with our modern sounds

      starsInDistance = true;            // True if stars move in distance behind maze, false if they are in fixed location
      diagnosticKeyDumpMode = false;     // True if want to dump keystrokes to the screen
      enableExperimentalAimMode = false; // True if we want to show experimental aiming vector in joystick mode

      showWeaponIndicators = true;       // True if we show the weapon indicators on the top of the screen
      verboseHelpMessages = true;        // If true, we'll show more handholding messages
      showKeyboardKeys = true;           // True if we show the keyboard shortcuts in joystick mode

      inputMode = Keyboard;              // Joystick or Keyboard
      masterAddress = "IP:67.18.11.66:25955";   // Default address of our master server
      name = "";                         // Player name (none by default)
      defaultName = "ChumpChange";       // Name used if user hits <enter> on name entry screen
      lastName = "ChumpChange";          // Name the user entered last time they ran the game
      hostname = "Bitfighter host";      // Default host name
      hostdescr = "";
      maxplayers = 128;                  // That's a lot of players!
      password = "";                     // Passwords empty by default
      adminPassword = "";
      levelChangePassword = "";
      levelDir = "";

      // Game window location when in windowed mode
      winXPos = 100;
      winYPos = 100;
      winSizeFact = 1.0;

      burstGraphicsMode = 1;
      szGraphicsMode = 1;
   }
};

void saveSettingsToINI();
void loadSettingsFromINI();

};

#endif

