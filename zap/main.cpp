////////////////////////
//To do prior to 1.0 release
//
////////////////////////
// Nitnoid
// Make color defs below constant... need to change associated externs too!

// Some time
// Add mouse coords to diagnostics screen, raw key codes

// Long term
// Admin select level w/ preview(?)

//Test:
// Do ships remember their spawn points?  How about robots?
// Does chat now work properly when ship is dead?  no
// Do LuaShip proxies remain constant over time (i.e. does 013 fix for Buvet.bot now work?)
// Make sure things work when ship is deleted.  Do we get nils back (and not crashes)?


// Ideas for interactive help/tutorial:
//=======================================
// Mine: Explodes when ships fly too close
// Beware: Enemy mines are hard to see!        (plant your own w/ the mine layer weapon)
// Teleport: takes you to another location on the map
// Friendly FF: Lets friendly ships pass
// Enemy FF: Lets enemy ships pass - destroy by shooting the base
// Neutral FF: Claim it for your team by repairing with the repair module
// Friendly Turret: Targets enemies, but won't hurt you (on purpose)
// Enemy Turret: Defends enemy teritory.  Destroy with multiple shots
// Neutral turret: Claim it for your team by repairing with the repair module
// Timer shows time left in game
// Heatlh indicator shows health left
// basic controls:  x/x/x/x to move; use 1,2,3 to select weapons; <c> shows overview map
// Messages will appear here -->
// See current game info by pressing [F2]


     


// Random point in zone, random zone, isInCaptureZone should return actual capture zone
// backport player count stuff

/*
XXX need to document timers, new luavec stuff XXX


/shutdown enhancements: on screen timer after msg dismissed, instant dismissal of local notice, notice in join menu, shutdown after level, auto shutdown when quitting and players connected


<ul>
<h4>UPGRADE INSTRUCTIONS</h4>
In Windows, Bitfighter will now attempt to put the level folders, the INI file, the screenshots folder, etc. in the folder pointed to by
"%LOCALAPPDATA%/Bitfighter", which, on Windows 7, typically maps to something like "C:\Users\Chris\AppData\Local\Bitfighter".  After installing 013 
for the first time, you should copy any levels, INI files, etc. to that folder, and overwriting any data that is there.

This change will resolve many installation and permissions issues.

<h4>Big changes</h4>
<li>Added energy item</li>
<li>Added ability to add arbitrary lin_ework to levels, complementing existing text.  To add lines, hold ~ and right-click.</li>
<li>Started trasnsition of Join menu into Game Lobby</li>
<li>Started developing framework for remote server management</li>

<h4>Smaller changes</h4>
<li>Added Restart Level to game options menu if you have level change permissions</li>
<li>Moved Add 2 Mins option to LevelChanger permission level (with older servers you'll still need admin rights)</li>
<li>Added asteroids, testItems, soccerball, and resourceItems to help screen, enhanced instruction colors</li>
<li>Fixed several in-game crashing issues</li>
<li>Minor efficiency improvements on rendering routines</li>
<li>Retrieve games now allow non-team flags</li>
<li>Bouncers now do half-damage to shooter, rather than full as in previous releases</li>
<li>Triples no longer damage shooter</li>
<li>Password entry in INI file now changed to ServerPassword -- if you use this, you'll need to manually update your INI file</li>
<li>HuntersFlagItem <x> <y> can no longer be used in level files.  Please use FlagItem -1 <x> <y> instead!!  Note that this line is only present in hand-modified level files</li>
<li>Min/max player level params now used when selecting levels</li>
<li>Slight improvement when rendering walls joining at acute angles</li>
<li>XBOX controller on windows now much better supported</li>
<li>Bot nav mesh zones can be displayed in-game by using /dzones command</li>
<li>Soccer ball now warps back to starting point rather than "drifting" back</li>
<li>Loadout/weapon indicators now more "vector graphicy", colors no longer botched on Vista/Win7</li>
<li>Infrastructure in place for goals with different values -- only need editor UI and level file syntax</li>

<h4>SFX</h4>
<li>New sound when ship hit by projectile</li>
<li>New sound when using GoFast</li>
<li>New SFX when players join/quit game</li>

<h4>Bots</h4>
<li>Added enhanced timer class to Lua scripting</li>
<li>Moved some "unwrapped" functionality into main() function</li>
<li>Renamed getMove() function to onTick() ==> existing bots will need to be updated</li>
<li>Fixed module "always on" bug</li>
<li>Fixed "shoot through shielded robot" bug</li>
<li>Event manager now working.  Can now add game events easily.</li>
<li>Added ShipSpawned, ShipKilled events</li>
<li>Added ship:isAlive() method</li>
<li>Return nil rather than dummy value when method called on dead ship (e.g. in 012, ship:getLoc() returned 0,0 if ship was dead, now returns nil)</li>
<li>Got rid of NexusFlagitem --> Now all flag are FlagItems, for all games</li>
<li>getName() function now run first, so game will have bot's name before any real game logic gets run</li>
<li>Created Player object with info about individual players/bots which you can get from Ship, GameInfo, and TeamInfo objects</li>
<li>Restructured the way bots are initialized so that all bots are loaded and getName()s are run before any main() functions are run</li>
<li>Can now pass arguments to bots from level file using arg struct, just like with levelgens</li>
<li>Robots now shown on scoreboard and in various menus where players are listed</li>
<li>Robot names are now "uniqued"</li>
<li>Overrode Lua's random generator to use one that's a bit more... random</li>
<li>Added setAngle() and setPolar() methods to point object</li>

<h4>Editor</h4>
<li>Many small improvements to editor</li>
<li>Fixed editor crashing issue caused by double-delete of item</li>
<li>Fixed editor crash while caused by pasting objects in the middle of a drag operation</li>
<li>Undo/redo more efficient -- will perform better with large levels and deep undo states (and will no longer crash!)</li>
<li>Added autosave -- will keep a current copy of the level on disk in the event of a crash (though crashes are much rarer now)</li>
<li>Ctrl-left click now starts a wall</li>
<li>Can now add arbitrary line items in editor: hold "~" while right-clicking to start</li>
<li>Editor remembers name of last edited file</li>
<li>Ctrl-A can now be used to hide navMeshZones</li>
<li>Improved preview mode (when holding Tab key)</li>
<li>Text in editor now subject to same min/max size constraints as text in the game</li>
<li>Team names can be changed in the editor</li>
<li>When moving, items now snap to grid based on vertex closest to cursor, giving much greater control over placement.  Snapping vertex highlighted in magenta.</li>
<li>Can now snap items to other items, not just to grid</li>
<li>Hovering over an item no longer counts as selecting it</li>
<li>Increased readibility of node numbers on selected lines/polygons</li>
<li>Holding tab now only shows range of selected turrets/spybugs</li>
<li>When editing navMeshZones, non-convex zones rendered in red to indicate potential problems</li>
<li>Improved snapping of nav mesh zones, including rendering of connections between zones</li>
<li>Grid fades when grid-snapping disabled</li>
<li>Changing wall widths is now undoable</li>
<li>Arbitrary scaling of selected items now works with Ctrl-Shift-X</li>
<li>There are now multiple edit modes (normal, bot nav mesh zone mode, etc.) customized for different purposes -- use Ctrl-A to toggle</li>
<li>Switched H and F key assignments -- F now inserts forcefield, H flips selection around horizontal axis</li>
<li>Holding <shift> while panning causes pan speed to increase -- good for larger levels at higher zoom scales</li>
<li>Improved coordinate display on dock -- now shows coords of snapping vertex when dragging an item</li>
<li>Editor now starts with (0,0) centered on the middle of the screen</li>

<h4>Server management</h4>
<li>Added /shutdown, /setlevpass, /setserverpass, and /setadminpass chat commands (see in-game help)</li>
<li>Added /setservername and /setserverdescr commands</li>
<li>Added /deletecurrentlevel command to remove level from remote server (file not deleted, but won't be loaded in future)</li>
<li>New orderly shutdown process when using /shutdown command (i.e. players notified, dedicated servers can be terminated, etc.)</li>
<li>If level change password is left blank, all players granted access (not so for admin password)</li>
<li>If level change password is set to blank, all players currently connected are silently granted access to level change menu</li>
<li>-password command line parameter changed to -serverpassword</li>
<li>When all players leave game, game advances to next level, and suspends itself until a player joins.  That way, when players join, level is "fresh" and ready to go.  May also reduce processor load and power consumption</li>
<li>Added ability to put game into suspended animation, automatically restarting when other players join (/suspend command)</li>

<h4>Linux</h4>
<li>Added ability to specify locations of various resouces on the cmd line.  See http://bitfighter.org/wiki/index.php?title=Command_line_parameters#Specifying_folders for details.</li>

<h4>Windows</h4>
<li>Windows installer now does a better job of installing files in their "proper" location</li>

<h4>Bugs</h4>
<li>Fixed rare Zap-era crash condition when player shoots a soccer ball, but quits game before goal is scored</li>
<li>Fixed turret/forcefield "in the middle of space" bug</li>
<li>Fixed textItem bounds miscalculation that caused large text to "jump" onto screen</li>
<li>Fixed /dcoord display position bug</li>
<li>Fixed robot bug reporting incorrect team for flags</li>
<li>Fixed extent bug when computing extents of levels that do not overlap (0,0)</li>

*/

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

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "IniFile.h"

#include "tnl.h"
#include "tnlRandom.h"
#include "tnlGhostConnection.h"
#include "tnlNetInterface.h"
#include "tnlJournal.h"

#ifdef TNL_OS_MAC_OSX
#include "Directory.h"
#endif

#include "zapjournal.h"

#include "../glut/glutInclude.h"
#include <stdarg.h>
#include <sys/stat.h>

using namespace TNL;

#include "UI.h"
#include "UIGame.h"
#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIEditor.h"
#include "UIErrorMessage.h"
#include "UIDiagnostics.h"
#include "UICredits.h"
#include "game.h"
#include "gameNetInterface.h"
#include "masterConnection.h"
#include "sfx.h"
#include "sparkManager.h"
#include "input.h"
#include "keyCode.h"
#include "config.h"
#include "md5wrapper.h"

#include "screenShooter.h"


#ifdef TNL_OS_MAC_OSX
#include <unistd.h>
#endif

namespace Zap
{

string gHostName;                // Server name used when hosting a game (default set in config.h, set in INI or on cmd line)
string gHostDescr;               // Brief description of host
const char *gWindowTitle = "Bitfighter";

// The following things can be set via command line parameters
S32 gMaxPlayers;                 // Max players allowed -- can change on cmd line, or INI.  Default value in config.h
U32 gSimulatedLag;               // Simulate a slow network -- can change on cmd line
F32 gSimulatedPacketLoss;        // Simulate a bad network -- can change on cmd line


#ifdef ZAP_DEDICATED
bool gDedicatedServer = true;    // This will allow us to omit the -dedicated parameter when compiled in dedicated mode
#else
bool gDedicatedServer = false;   // Usually, we just want to play.  If true, we'll be in server-only, no-player mode
#endif

bool gQuit = false;
bool gIsServer = false;

// Handle any md5 requests
md5wrapper md5;


bool gShowAimVector = false;     // Do we render an aim vector?  This should probably not be a global, but until we find a better place for it...
bool gDisableShipKeyboardInput;  // Disable ship movement while user is in menus

U32 gUseStickNumber = 1;         // Which joystick do you want to use (1 = first, which is typical)
U32 gSticksFound = 0;            // Which joystick we're actually using...

CIniFile gINI("dummy");          // This is our INI file.  Filename set down in main(), but compiler seems to want an arg here.

CmdLineSettings gCmdLineSettings;

ConfigDirectories gConfigDirs;

IniSettings gIniSettings;

ControllerTypeType gAutoDetectedJoystickType;   // Remember what sort of joystick was found for diagnostic purposes


// Some colors -- other candidates include global and local chat colors, which are defined elsewhere.  Include here?
Color gNexusOpenColor(0, 0.7, 0);
Color gNexusClosedColor(0.85, 0.3, 0);
Color gErrorMessageTextColor(1, 0.5, 0.5);
Color gNeutralTeamColor(0.8, 0.8, 0.8);         // Objects that are neutral (on team -1)
Color gHostileTeamColor(0.5, 0.5, 0.5);         // Objects that are "hostile-to-all" (on team -2)
Color gMasterServerBlue(0.8, 0.8, 1);           // Messages about successful master server statii
Color gHelpTextColor(0, 1, 0);
Color WALL_OUTLINE_COLOR(0, 0, 1);
Color GAME_WALL_FILL_COLOR(0, 0, 0.15);         // Walls filled with this in game
Color EDITOR_WALL_FILL_COLOR(.5, .5, 1);        // Walls filled with this in editor

S32 gMaxPolygonPoints = 32;                     // Max number of points we can have in Nexuses, LoadoutZones, etc.

bool gReadyToConnectToMaster = false;           // When we're sure we have a nickname, we'll set this to true and proceed to connect to the master

string gServerPassword = "";
string gAdminPassword = "";
string gLevelChangePassword = "";

Address gMasterAddress;
Address gConnectAddress;
Address gBindAddress(IPProtocol, Address::Any, 28000);      // Good for now, may be overwritten by INI or cmd line setting
      // Above is equivalent to ("IP:Any:28000")

Vector<StringTableEntry> gLevelList;      // Levels we'll play when we're hosting
Vector<StringTableEntry> gLevelSkipList;  // Levels we'll never load, to create a semi-delete function for remote server mgt

// Lower = more slippery!  Not used at the moment...
F32 gNormalFriction = 1000;   // Friction between vehicle and ground, ordinary
F32 gSlipFriction = 400;      // Friction, on a slip square

char gJoystickName[gJoystickNameLength] = "";

extern Point gMousePos;

// Since GLUT reports the current mouse pos via a series of events, and does not make
// its position available upon request, we'll store it when it changes so we'll have
// it when we need it.
void setMousePos(S32 x, S32 y)
{
   gMousePos.x = x;
   gMousePos.y = y;
}

Screenshooter gScreenshooter;    // For taking screen shots

ZapJournal gZapJournal;    // Our main journaling object

// Handler called by GLUT when window is reshaped
void GLUT_CB_reshape(int nw, int nh)
{
   gZapJournal.reshape(nw, nh);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, reshape, (S32 newWidth, S32 newHeight), (newWidth, newHeight))
{
   // If we are entering fullscreen mode, then we don't want to mess around with proportions and all that.  Just save window size and get out.
   if(gIniSettings.fullscreen)
   {
      // The following block will attempt to keep graphics from being stretched on a monitor with non-standard proportions
      // It works, but the effect is worse than the stretching, in my opinion.
      //F32 fact;
      //if((newWidth - UserInterface::canvasWidth) > (newHeight - UserInterface::canvasHeight))
      //   fact = max((F32) newHeight / (F32) UserInterface::canvasHeight, 0.15f);
      //else
      //   fact = max((F32) newWidth / (F32) UserInterface::canvasWidth, 0.15f);

      //newHeight = UserInterface::canvasHeight * fact;
      //newWidth = UserInterface::canvasWidth * fact;

      UserInterface::windowWidth = newWidth;
      UserInterface::windowHeight = newHeight;
      return;
   }

   // Constrain window to correct proportions...
   if((newWidth - UserInterface::canvasWidth) > (newHeight - UserInterface::canvasHeight))
      gIniSettings.winSizeFact = max((F32) newHeight / (F32) UserInterface::canvasHeight, 0.15f);
   else
      gIniSettings.winSizeFact = max((F32) newWidth / (F32) UserInterface::canvasWidth, 0.15f);

   newHeight = (S32)(UserInterface::canvasHeight * gIniSettings.winSizeFact);
   newWidth  = (S32)(UserInterface::canvasWidth  * gIniSettings.winSizeFact);

   glutReshapeWindow(newWidth, newHeight);

   UserInterface::windowWidth = newWidth;
   UserInterface::windowHeight = newHeight;

   gINI.SetValueF("Settings", "WindowScalingFactor", gIniSettings.winSizeFact, true);
}

// Handler called by GLUT when mouse motion is detected
void GLUT_CB_motion(int x, int y)
{
   gZapJournal.motion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, motion, (S32 x, S32 y), (x, y))
{
   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseDragged(x, y);
}

// Handler called by GLUT when "passive" mouse motion is detected
void GLUT_CB_passivemotion(int x, int y)
{
   gZapJournal.passivemotion(x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, passivemotion, (S32 x, S32 y), (x, y))
{

   // Glut sometimes fires spurious events.  Let's ignore those.
   if(x == gMousePos.x && y == gMousePos.y)
      return;

   setMousePos(x, y);

   if(UserInterface::current)
      UserInterface::current->onMouseMoved(x, y);
}

void keyDown(KeyCode keyCode, char ascii)    // Launch the onKeyDown event
{
   if(UserInterface::current)
      UserInterface::current->onKeyDown(keyCode, ascii);
}

// Sometimes we need to pretend a key was pressed, such as for those that don't
// generate events (shift/ctrl/alt, controller buttons, etc.)
void simulateKeyDown(KeyCode keyCode)
{
   setKeyState(keyCode, true);
   keyDown(keyCode, 0);
}

void keyUp(KeyCode keyCode)              // Launch the onKeyUp event
{
   if(UserInterface::current)
      UserInterface::current->onKeyUp(keyCode);
}

void simulateKeyUp(KeyCode keyCode)
{
   setKeyState(keyCode, false);
   keyUp(keyCode);
}

// GLUT handler for key-down events
void GLUT_CB_keydown(unsigned char key, S32 x, S32 y)
{
   gZapJournal.keydown(key);
}


TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keydown, (U8 key), (key))
{
   // First check for some "universal" keys.  If keydown isn't one of those, we'll pass the key onto the keyDown handler
   // Check for ALT-ENTER --> toggles window mode/full screen
   if(key == '\r' && (glutGetModifiers() & GLUT_ACTIVE_ALT))
      gOptionsMenuUserInterface.toggleFullscreen();
   else if(key == 17)      // GLUT reports Ctrl-Q as 17
      gScreenshooter.phase = 1;
   else
   {
      KeyCode keyCode = standardGLUTKeyToKeyCode(key);
      setKeyState(keyCode, true);
      keyDown(keyCode, keyToAscii(key, keyCode));
   }
}

#ifndef ZAP_DEDICATED

// GLUT handler for key-up events
void GLUT_CB_keyup(unsigned char key, int x, int y)
{
   gZapJournal.keyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, keyup, (U8 key), (key))
{
   KeyCode keyCode = standardGLUTKeyToKeyCode(key);
   setKeyState(keyCode, false);
   keyUp(keyCode);
}

// GLUT handler for mouse clicks
void GLUT_CB_mouse(int button, int state, int x, int y)
{
   gZapJournal.mouse(button, state, x, y);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, mouse, (S32 button, S32 state, S32 x, S32 y), (button, state, x, y))
{
   setMousePos(x, y);

   if(!UserInterface::current) return;    // Bail if no current UI

   if(button == GLUT_LEFT_BUTTON)
   {
      setKeyState(MOUSE_LEFT, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_LEFT, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_LEFT);
   }
   else if(button == GLUT_RIGHT_BUTTON)
   {
      setKeyState(MOUSE_RIGHT, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_RIGHT, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_RIGHT);
   }
   else if(button == GLUT_MIDDLE_BUTTON)
   {
      setKeyState(MOUSE_MIDDLE, (state == GLUT_DOWN));

      if(state == GLUT_DOWN)
         keyDown(MOUSE_MIDDLE, 0);
      else // state == GLUT_UP
         keyUp(MOUSE_MIDDLE);
   }
}

// GLUT callback for special key down (special keys are things like F1-F12)
void GLUT_CB_specialkeydown(int key, int x, int y)
{
   gZapJournal.specialkeydown(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeydown, (S32 key), (key))
{
   KeyCode keyCode = specialGLUTKeyToKeyCode(key);
   setKeyState(keyCode, true);

   if(keyCode == keyDIAG && !gDiagnosticInterface.isActive())   // Turn on diagnostic overlay if not already on
   {
      UserInterface::playBoop();
      gDiagnosticInterface.activate();
   }
   else
      keyDown(keyCode, keyToAscii(key, keyCode));           // Launch onKeyDown event
}


// GLUT callback for special key up
void GLUT_CB_specialkeyup(int key, int x, int y)
{
   gZapJournal.specialkeyup(key);
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, specialkeyup, (S32 key), (key))
{
   KeyCode keyCode = specialGLUTKeyToKeyCode(key);
   setKeyState(keyCode, false);
   keyUp(keyCode);
}
#endif

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeydown, (U32 key), (key))
{
   if(key == 0)         // shift
   {
      setKeyState(KEY_SHIFT, true);
      keyDown(KEY_SHIFT, 0);
   }
   else if(key == 1)    // ctrl
   {
      setKeyState(KEY_CTRL, true);
      keyDown(KEY_CTRL, 0);
   }
   else if(key == 2)    // alt
   {
      setKeyState(KEY_ALT, true);
      keyDown(KEY_ALT, 0);
   }
}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, modifierkeyup, (U32 key), (key))
{
   if(key == 0)         // shift
   {
      setKeyState(KEY_SHIFT, false);
      keyUp(KEY_SHIFT);
   }
   else if(key == 1)    // ctrl
   {
      setKeyState(KEY_CTRL, false);
      keyUp(KEY_CTRL);
   }
   else if(key == 2)    // alt
   {
      setKeyState(KEY_ALT, false);
      keyUp(KEY_ALT);
   }
}


void exitGame(S32 errcode)
{
   #ifdef TNL_OS_XBOX
      extern void xboxexit();
      xboxexit();
   #else
      exit(errcode);
   #endif
}


// Exit the game, back to the OS
void exitGame()
{
   exitGame(0);
}


// If we can't load any levels, here's the plan...
void abortHosting()
{
   if(gDedicatedServer)
   {
      logprintf("No levels found in folder %s.  Cannot host a game.", gConfigDirs.levelDir.c_str());
      s_logprintf("No levels found in folder %s.  Cannot host a game.", gConfigDirs.levelDir.c_str());
      //printf("No levels were loaded from folder %s.  Cannot host a game.", gLevelDir.c_str());      ==> Does nothing
      exitGame(1);
   }
   else
   {
      gErrorMsgUserInterface.reset();
      gErrorMsgUserInterface.setTitle("HOUSTON, WE HAVE A PROBLEM");
      gErrorMsgUserInterface.setMessage(1, "No levels were loaded.  Cannot host a game.");
      gErrorMsgUserInterface.setMessage(3, "Check the LevelDir parameter in your INI file,");
      gErrorMsgUserInterface.setMessage(4, "or your command-line parameters to make sure");
      gErrorMsgUserInterface.setMessage(5, "you have correctly specified a folder containing");
      gErrorMsgUserInterface.setMessage(6, "valid level files.");
      gErrorMsgUserInterface.setMessage(8, "Trying to load levels from folder:");
      gErrorMsgUserInterface.setMessage(9, gConfigDirs.levelDir.c_str());
      gErrorMsgUserInterface.activate();
   }
   delete gServerGame;
   gServerGame = NULL;

   gMainMenuUserInterface.levelLoadDisplayDisplay = false;
   gMainMenuUserInterface.levelLoadDisplayFadeTimer.clear();

   return;
}


// Host a game (and maybe even play a bit, too!)
void initHostGame(Address bindAddress, bool testMode)
{
   gServerGame = new ServerGame(bindAddress, gMaxPlayers, gHostName.c_str(), testMode);

   // Don't need to build our level list when in test mode because we're only running that one level stored in editor.tmp
   if(!testMode)
   {
      s_logprintf("----------\nbitfighter server started [%s]", getTimeStamp().c_str());
      s_logprintf("hostname=[%s], hostdescr=[%s]", gServerGame->getHostName(), gServerGame->getHostDescr());

      LevelListLoader::buildLevelList();     // Populates gLevelList

      s_logprintf("Loaded %d levels:", gLevelList.size());
   }

   // Parse all levels, make sure they are in some sense valid, and record some critical parameters
   if(gLevelList.size())
   {
      gServerGame->setLevelList(gLevelList);
      gServerGame->resetLevelLoadIndex();
      gMainMenuUserInterface.levelLoadDisplayDisplay = true;
   }
   else
   {
      abortHosting();
      return;
   }

  // Do this even if there are no levels, so hostGame error handling will be triggered
  gServerGame->hostingModePhase = ServerGame::LoadingLevels;  
}


void hostGame()
{
   gServerGame->hostingModePhase = ServerGame::Hosting;

   for(S32 i = 0; i < gServerGame->getLevelNameCount(); i++)
      s_logprintf("\t%s [%s]", gServerGame->getLevelNameFromIndex(i).getString(), gServerGame->getLevelFileNameFromIndex(i).c_str());

   if(gServerGame->getLevelNameCount())             // Levels loaded --> start game!
      gServerGame->cycleLevel(ServerGame::FIRST_LEVEL);   // Start the first level

   else        // No levels loaded... we'll crash if we try to start a game
   {
      abortHosting();
      return;
   }

   gMainMenuUserInterface.levelLoadDisplayDisplay = false;
   gMainMenuUserInterface.levelLoadDisplayFadeTimer.reset();

   if(!gDedicatedServer)                  // If this isn't a dedicated server...
      joinGame(Address(), false, true);   // ...then we'll play, too!
      //      (let the system assign ip and port, false -> not from master, true -> local connection)
   //else
   //   printf("Bitfighter host launched.\n");     ==> Does nothing
}


// This is the master idle loop that gets registered with GLUT and is called on every game tick.
// This in turn calls the idle functions for all other objects in the game.
void idle()
{
   if(gServerGame)
   {
      if(gServerGame->hostingModePhase == ServerGame::LoadingLevels)
         gServerGame->loadNextLevel();
      else if(gServerGame->hostingModePhase == ServerGame::DoneLoadingLevels)
         hostGame();
   }

   checkModifierKeyState();      // Most keys are handled as events by GLUT...  but not Ctrl, Alt, Shift!
   static S64 lastTimer = Platform::getHighPrecisionTimerValue();
   static F64 unusedFraction = 0;

   S64 currentTimer = Platform::getHighPrecisionTimerValue();

   F64 timeElapsed = Platform::getHighPrecisionMilliseconds(currentTimer - lastTimer) + unusedFraction;
   U32 integerTime = U32(timeElapsed);

   if(integerTime >= 10)         // Thus max frame rate = 100
   {
      lastTimer = currentTimer;
      unusedFraction = timeElapsed - integerTime;

      gZapJournal.idle(integerTime);
   }

   // So, what's with all the SDL code in here?  I looked at converting from GLUT to SDL, in order to get
   // a richer set of keyboard events.  Looks possible, but SDL appears to be missing some very handy
   // windowing code (e.g. the ability to resize or move a window) that GLUT has.  So until we find a
   // platform independent window library, we'll stick with GLUT, or maybe go to FreeGlut.
   // Note that moving to SDL will require our journaling system to be re-engineered.
   // Note too that SDL will require linking in SDL.lib and SDLMain.lib, and including the SDL.dll in the EXE folder.

   /* SDL requires an active polling loop.  We could use something like the following:
   while(SDL_PollEvent(&e))
   {
      switch(e.type)
      {
         case SDL_KEYDOWN:
            gZapJournal.keydown((S32) e.key.keysym.sym);      // Cast to S32 to ensure journaling system can cope
            break;
         case SDL_KEYUP:
            gZapJournal.keyup((S32) e.key.keysym.sym);
            break;
         case SDL_MOUSEMOTION:
            break;
         case SDL_VIDEORESIZE:
            window_resized(e.resize.w, e.resize.h);
            break;
         case SDL_QUIT:       // User closed game window
            exitGame();
            break;
      }
   }

   gZapJournal.display();    // Draw the screen --> GLUT handles this via callback, with SDL we need to do it in our main loop
   END SDL event polling */

   // Sleep a bit so we don't saturate the system. For a non-dedicated server,
   // sleep(0) helps reduce the impact of OpenGL on windows.
   U32 sleepTime = 1;

   if(gClientGame) sleepTime = 0;      // Live player at the console


   // If there are no players, set sleepTime to 40 to further reduce impact on the server.
   // We'll only go into this longer sleep on dedicated servers when there are no players.
   if(gDedicatedServer && gServerGame->isSuspended())
      sleepTime = 40;

   Platform::sleep(sleepTime);

   gZapJournal.processNextJournalEntry();    // Does nothing unless we're playing back a journal...

}  // end idle()



TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, idle, (U32 integerTime), (integerTime))
{
   if(UserInterface::current)
      UserInterface::current->idle(integerTime);

   if(!(gServerGame && gServerGame->hostingModePhase == ServerGame::LoadingLevels))    // Don't idle games during level load
   {
      if(gClientGame)
         gClientGame->idle(integerTime);
      if(gServerGame)
         gServerGame->idle(integerTime);
   }

   if(gClientGame)
      glutPostRedisplay();
}

void dedicatedServerLoop()
{
   for(;;)        // Loop forever!
      idle();     // Idly!
}

#ifndef ZAP_DEDICATED
void GLUT_CB_display(void)
{
   gZapJournal.display();

   if(gScreenshooter.phase)      // We're in mid-shot, so be sure to visit the screenshooter!
      gScreenshooter.saveScreenshot();
}
#endif

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, display, (), ())
{
   glFlush();
   UserInterface::renderCurrent();

   // Render master connection state if we're not connected
   if(gClientGame && gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->getConnectionState() != NetConnection::Connected)
   {
      glColor3f(1, 1, 1);
      UserInterface::drawStringf(10, 550, 15, "Master Server - %s", gConnectStatesTable[gClientGame->getConnectionToMaster()->getConnectionState()]);
   }

   // Swap the buffers. This this tells the driver to render the next frame from the contents of the
   // back-buffer, and to set all rendering operations to occur on what was the front-buffer.
   // Double buffering prevents nasty visual tearing from the application drawing on areas of the
   // screen that are being updated at the same time.
   glutSwapBuffers();
   //SDL_GL_SwapBuffers();  // Use this if we convert to SDL
 }


string joindir(string path, string filename)
{
   return (path == "" ? filename : path + "/" + filename);
}


#include <stdio.h>
// Each instnatiation of a LogConsumer subclass gets a copy of all log messages.  Here we'll log both
// to the screen as well as to a file called bitfighter.log
class StdoutLogConsumer : public LogConsumer   // Dumps logs to stdout
{
public:
   void logString(const char *string)
   {
      printf("%s", string);
   }
} gStdoutLogConsumer;


class FileLogConsumer : public LogConsumer     // Dumps logs to bitfighter.log
{
protected:
   FILE *f;
   const char *filename;
   const char *mode;
   const char *firstLine;
   bool isOpen;  // RDW - This was "boolean" (CE - Too much Java!)

public:
   FileLogConsumer(const char* logFile="bitfighter.log")
   {
      filename = logFile;
      isOpen = false;
      mode = "w";
      firstLine = "------ Bitfighter Log File ------\n";
   }

   ~FileLogConsumer()
   {
      if(f)
         fclose(f);
   }

   void open()
   {
      f = fopen(joindir(gConfigDirs.logDir, filename).c_str(), mode);
      isOpen = true;    // Set this here to avoid endless loops when the next line is executed!
      logString(firstLine);
   }

   void logString(const char *string)
   {
      // Do this whole lazy initialize thing to ensure that we've read our command line and know where to
      // create the logfile by the time we get here.
      if(!isOpen)
         open();

      if(f)
      {
         fprintf(f, "%s", string);
         fflush(f);
      }
   }
} gFileLogConsumer;


class ServerFileLogConsumer : public FileLogConsumer    // Dumps logs to bitfighter_server.log
{
public:
   ServerFileLogConsumer(const char* logFile="bitfighter_server.log")
   {
      filename = logFile;
      isOpen = false;
      mode = "a";
      firstLine = "";

      setFilterType(LogConsumer::ServerFilter);
   }

} gServerLogConsumer;


// Player has selected a game from the QueryServersUserInterface, and is ready to join
void joinGame(Address remoteAddress, bool isFromMaster, bool local)
{
   if(isFromMaster && gClientGame->getConnectionToMaster())
   {
      gClientGame->getConnectionToMaster()->requestArrangedConnection(remoteAddress);
      gGameUserInterface.activate();
   }
   else
   {
      GameConnection *theConnection = new GameConnection();
      gClientGame->setConnectionToServer(theConnection);

      // Use name specified in name entry screen, falling back to defaultName (from INI) if blank
      const char *name = gNameEntryUserInterface.getText()[0] ? gNameEntryUserInterface.getText() : gIniSettings.defaultName.c_str();

      theConnection->setClientName(name);

      theConnection->setSimulatedNetParams(gSimulatedPacketLoss, gSimulatedLag);

      if(local)   // Local client
      {
         theConnection->connectLocal(gClientGame->getNetInterface(), gServerGame->getNetInterface());
         theConnection->setIsAdmin(true);          // Local connection is always admin
         theConnection->setIsLevelChanger(true);   // Local connection can always change levels

         GameConnection *gc = dynamic_cast<GameConnection *>(theConnection->getRemoteConnectionObject());

         if(gc)                              // gc might evaluate false if a bad password was supplied to a password-protected server
         {
            gc->setIsAdmin(true);            // Set isAdmin on server
            gc->setIsLevelChanger(true);     // Set isLevelChanger on server

            gc->s2cSetIsAdmin(true);                // Set isAdmin on the client
            gc->s2cSetIsLevelChanger(true, false);  // Set isLevelChanger on the client
            gc->setServerName(gServerGame->getHostName());     // Server name is whatever we've set locally
         }
      }
      else        // Remote client
         theConnection->connect(gClientGame->getNetInterface(), remoteAddress);  // (method in tnlNetConnection)

      gGameUserInterface.activate();
   }
}

// Disconnect from servers and exit game in an orderly fashion
void endGame()
{
   // Disconnect from master server
   if(gClientGame && gClientGame->getConnectionToMaster())
      gClientGame->getConnectionToMaster()->cancelArrangedConnectionAttempt();

   // Disconnect from game server
   if(gClientGame && gClientGame->getConnectionToServer())
      gClientGame->getConnectionToServer()->disconnect(NetConnection::ReasonSelfDisconnect, "");

   delete gServerGame;
   gServerGame = NULL;

   if(gDedicatedServer)
      exitGame();
}


// Run when we're quitting the game
void onExit()
{
   endGame();
   SFXObject::shutdown();
   ShutdownJoystick();

   // Save settings to capture window position
   gINI.SetValue("Settings", "WindowMode", (gIniSettings.fullscreen ? "Fullscreen" : "Window"), true);
   if(!gIniSettings.fullscreen)
   {
      gINI.SetValueI("Settings", "WindowXPos", glutGet(GLUT_WINDOW_X), true);
      gINI.SetValueI("Settings", "WindowYPos", glutGet(GLUT_WINDOW_Y), true);
   }

   gINI.WriteFile();

   NetClassRep::logBitUsage();
   TNL::logprintf("Bye!");

   exitGame();
}


// If we're running in dedicated mode, these things need to be set as such.
void setParamsForDedicatedMode()
{
   gCmdLineSettings.clientMode = false;
   gCmdLineSettings.serverMode = true;
   gDedicatedServer = true;
   gReadyToConnectToMaster = true;

   gCmdLineSettings.connectRemote = false;
}



// Read the command line params... if we're replaying a journal, we'll process those params as if they were actually there, while
// ignoring those params that were provided.
TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, readCmdLineParams, (Vector<StringPtr> argv), (argv))
{
   S32 argc = argv.size();

   // Process command line args  --> see http://bitfighter.org/wiki/index.php?title=Command_line_parameters
   for(S32 i = 0; i < argc; i+=2)
   {
      bool hasAdditionalArg = (i != argc - 1 && argv[i + 1].getString()[0] != '-');     // Assume "args" starting with "-" are actually follow-on params
      bool has2AdditionalArgs = hasAdditionalArg && (i != argc - 2);

      // Connect to a game server
      if(!stricmp(argv[i], "-connect"))       // additional arg required
      {
         if(hasAdditionalArg)
         {
            gCmdLineSettings.connectRemote = true;
            gCmdLineSettings.connect = argv[i+1];
         }
         else
         {
            logprintf("You must specify a server address to connect to with the -connect option");
            exitGame(1);
         }
      }
      // Specify a master server
      else if(!stricmp(argv[i], "-master"))        // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.masterAddress = argv[i+1];
         else
         {
            logprintf("You must specify a master server address with -master option");
            exitGame(1);
         }
      }
      // Address to use when we're hosting
      else if(!stricmp(argv[i], "-hostaddr"))       // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostaddr = argv[i+1];
         else
         {
            logprintf("You must specify a host address for the host to listen on (e.g. IP:Any:28000 or IP:192.169.1.100:5500)");
            exitGame(1);
         }
      }
      // Simulate packet loss 0 (none) - 1 (total)  [I think]
      else if(!stricmp(argv[i], "-loss"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.loss = atof(argv[i+1]);
         else
         {
            logprintf("You must specify a loss rate between 0 and 1 with the -loss option");
            exitGame(1);
         }
      }
      // Simulate network lag
      else if(!stricmp(argv[i], "-lag"))           // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.lag = atoi(argv[i+1]);
         else
         {
            logprintf("You must specify a lag (in ms) with the -lag option");
            exitGame(1);
         }
      }
      // Run as a dedicated server
      else if(!stricmp(argv[i], "-dedicated"))     // additional arg optional
      {
         setParamsForDedicatedMode();

         if(hasAdditionalArg)
            gCmdLineSettings.dedicated = argv[i+1];
         else
            i--;     // Correct for the fact that we don't really have two args here...
      }
      // Specify user name
      else if(!stricmp(argv[i], "-name"))          // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.name = argv[i+1];
         else
         {
            logprintf("You must enter a nickname with the -name option");
            exitGame(1);
         }
      }
      // Specify password for accessing locked ser vers
      else if(!stricmp(argv[i], "-serverpassword"))      // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.serverPassword = argv[i+1];
         else
         {
            logprintf("You must enter a password with the -serverpassword option");
            exitGame(1);
         }
      }
      // Specify admin password for server
      else if(!stricmp(argv[i], "-adminpassword")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.adminPassword = argv[i+1];
         else
         {
            logprintf("You must specify an admin password with the -adminpassword option");
            exitGame(1);
         }
      }
      // Specify level change password for server
      else if(!stricmp(argv[i], "-levelchangepassword")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.levelChangePassword = argv[i+1];
         else
         {
            logprintf("You must specify an level-change password with the -levelchangepassword option");
            exitGame(1);
         }
      }
      // Specify to include all levels in levels folder -- not really needed any more, but can be used as a shortcut to tell game to ignore levels in INI file...
      else if(!stricmp(argv[i], "-alllevels"))     // no additional args
      {
         i--;  // compentsate for +=2 in for loop with single param
         gCmdLineSettings.alllevels = true;
      }

      else if(!stricmp(argv[i], "-rootdatadir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify the robots folder with the -rootdatadir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.rootDataDir = argv[i+1].getString();
      }


      else if(!stricmp(argv[i], "-leveldir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify a levels subfolder with the -leveldir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.levelDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-inidir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify a the folder where your INI file is stored with the -inidir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.iniDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-logdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify your log folder with the -logdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.logDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-luadir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify the folder where the Lua helper scripts are stored with the -luadir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.luaDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-robotdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify the robots folder with the -robotdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.robotDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-screenshotdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify your screenshots folder with the -screenshotdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.screenshotDir = argv[i+1].getString();
      }

      else if(!stricmp(argv[i], "-sfxdir"))      // additional arg required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify your sounds folder with the -sfxdir option");
            exitGame(1);
         }

         gCmdLineSettings.dirs.sfxDir = argv[i+1].getString();
      }


      // Specify list of levels...  all remaining params will be taken as level names
      else if(!stricmp(argv[i], "-levels"))     // additional arg(s) required
      {
         if(!hasAdditionalArg)
         {
            logprintf("You must specify one or more levels to load with the -levels option");
            exitGame(1);
         }

         // We'll overwrite our main level list directly, so if we're writing the INI for the first time,
         // we'll use the cmd line args to generate the INI Level keys, rather than the built-in defaults.
         for(S32 j = i+1; j < argc; j++)
            gCmdLineSettings.specifiedLevels.push_back(StringTableEntry(argv[j]));

         return;     // This param must be last, so no more args to process.  We can return.

      }
      // Specify name of the server as others will see it from the Join menu
      else if(!stricmp(argv[i], "-hostname"))   // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostname = argv[i+1];
         else
         {
            logprintf("You must specify a server name with the -hostname option");
            exitGame(1);
         }
      }
      else if(!stricmp(argv[i], "-hostdescr"))   // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.hostdescr = argv[i+1];
         else
         {
            logprintf("You must specify a description (use quotes) with the -hostdescr option");
            exitGame(1);
         }
      }
      // Change max players on server
      else if(!stricmp(argv[i], "-maxplayers")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.maxplayers = atoi(argv[i+1]);
         else
         {
            logprintf("You must specify the max number of players on your server with the -maxplayers option");
            exitGame(1);
         }
      }
      // Start in window mode
      else if(!stricmp(argv[i], "-window"))     // no additional args
      {
         i--;  // compentsate for +=2 in for loop with single param
         gCmdLineSettings.window = true;
      }
      // Start in fullscreen mode
      else if(!stricmp(argv[i], "-fullscreen")) // no additional args
      {
         i--;
         gCmdLineSettings.fullscreen = true;
      }
      // Specify position of window
      else if(!stricmp(argv[i], "-winpos"))     // 2 additional args required
      {
         if(has2AdditionalArgs)
         {
            gCmdLineSettings.xpos = atoi(argv[i+1]);
            gCmdLineSettings.ypos = atoi(argv[i+2]);
            i++;  // compentsate for +=2 in for loop with single param (because we're grabbing two)
         }
         else
         {
            logprintf("You must specify the x and y position of the window with the -winpos option");
            exitGame(1);
         }
      }
      // Specify width of the game window
      else if(!stricmp(argv[i], "-winwidth")) // additional arg required
      {
         if(hasAdditionalArg)
            gCmdLineSettings.winWidth = atoi(argv[i+1]);
         else
         {
            logprintf("You must specify the width of the game window with the -winwidth option");
            exitGame(1);
         }
      }
      else if(!stricmp(argv[i], "-help"))       // no additional args
      {
         i--;
         logprintf("See http://bitfighter.org/wiki/index.php?title=Command_line_parameters for information");
         exitGame(0);
      }
      // Highly speculative use of different joysticks
      else if(!stricmp(argv[i], "-usestick")) // additional arg required
      {
         if(hasAdditionalArg)
            gUseStickNumber = atoi(argv[i+1]);           /////////////////////////////////////////  TODO: should be part of gCmdLineSettings
         else
         {
            logprintf("You must specify the joystick you want to use with the -usestick option");
            exitGame(1);
         }
      }
   }

// Override some settings if we're compiling ZAP_DEDICATED
#ifdef ZAP_DEDICATED
   setParamsForDedicatedMode();
#endif
}

/*
void InitSdlVideo()
{
   // Information about the current video settings.
   const SDL_VideoInfo* info = NULL;

   // Flags we will pass into SDL_SetVideoMode.
   S32 flags = 0;

   // First, initialize SDL's video subsystem.
   if (SDL_Init(SDL_INIT_VIDEO) < 0)
   {
       // Failed, exit.
       logprintf("SDL Video initialization failed: %s", SDL_GetError( ));
       exitGame();
   }

   // Let's get some video information.
   info = SDL_GetVideoInfo( );

   if( !info ) {
       // This should probably never happen.
       logprintf("SDL Video query failed: %s", SDL_GetError());
       exitGame();
   }

   // We get the bpp we will request from
   // the display. On X11, VidMode can't change
   // resolution, so this is probably being overly
   // safe. Under Win32, ChangeDisplaySettings
   // can change the bpp.

   gBPP = info->vfmt->BitsPerPixel;

   // Now, we want to setup our requested
   // window attributes for our OpenGL window.
   // We want *at least* 5 bits of red, green
   // and blue. We also want at least a 16-bit
   // depth buffer.
   //
   // The last thing we do is request a double
   // buffered window. '1' turns on double
   // buffering, '0' turns it off.
   //
   // Note that we do not use SDL_DOUBLEBUF in
   // the flags to SDL_SetVideoMode. That does
   // not affect the GL attribute state, only
   // the standard 2D blitting setup.

   SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
   SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
   SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

   // We want to request that SDL provide us with an OpenGL window, possibly in a fullscreen video mode.
   // Note the SDL_DOUBLEBUF flag is not required to enable double buffering when setting an OpenGL
   // video mode. Double buffering is enabled or disabled using the SDL_GL_DOUBLEBUFFER attribute.
   flags = SDL_OPENGL | SDL_RESIZABLE ; // | SDL_FULLSCREEN;


   if(SDL_SetVideoMode(gScreenWidth, gScreenHeight, gBPP, flags ) == 0)
   {
      // This could happen for a variety of reasons,
      // including DISPLAY not being set, the specified
      // resolution not being available, etc.

      logprintf("SDL Video mode set failed: %s", SDL_GetError());
      exitGame();
   }

   SDL_WM_SetCaption(gWindowTitle, "Icon XXX");    // TODO: Fix icon here
}
*/


// Basically checks if the folder base exists, and if not, makes it a subdir of levels
// Typos on the user's part can lead to hilarity!
string getLevelsFolder(string base)
{
   // See if levelsFolder could refer to a standalone folder (rather than a subfolder of gLevelDir)
   struct stat st;
   if(stat(base.c_str(), &st) != 0 )
      return gConfigDirs.levelDir + "/" + base;      // It doesn't
   else
      return base;                                   // It does
}


// Now integrate INI settings with those from the command line and process them
void processStartupParams()
{
   // These options can only be set on cmd line
   if(!gCmdLineSettings.server.empty())
      gBindAddress.set(gCmdLineSettings.server);

   if(!gCmdLineSettings.dedicated.empty())
      gBindAddress.set(gCmdLineSettings.dedicated);

   if(gCmdLineSettings.connect != "")
      gConnectAddress.set(gCmdLineSettings.connect);

   gSimulatedPacketLoss = gCmdLineSettings.loss;
   gSimulatedLag = gCmdLineSettings.lag;

   // Enable some logging...
   TNLLogEnable(LogConnectionProtocol, gIniSettings.logConnectionProtocol);
   TNLLogEnable(LogNetConnection, gIniSettings.logNetConnection);
   TNLLogEnable(LogEventConnection, gIniSettings.logEventConnection);
   TNLLogEnable(LogGhostConnection, gIniSettings.logGhostConnection);

   TNLLogEnable(LogNetInterface, gIniSettings.logNetInterface);
   TNLLogEnable(LogPlatform, gIniSettings.logPlatform);
   TNLLogEnable(LogNetBase, gIniSettings.logNetBase);
   TNLLogEnable(LogUDP, gIniSettings.logUDP);


   // These options can come either from cmd line or INI file
   if(gCmdLineSettings.name != "")
      gNameEntryUserInterface.setString(gCmdLineSettings.name);
   else if(gIniSettings.name != "")
      gNameEntryUserInterface.setString(gIniSettings.name);
   else
      gNameEntryUserInterface.setString(gIniSettings.lastName);

   // Put any saved filename into the editor file entry thingy
   gLevelNameEntryUserInterface.setString(gIniSettings.lastEditorName);


   if(gCmdLineSettings.serverPassword != "")
      gServerPassword = gCmdLineSettings.serverPassword;
   else if(gIniSettings.serverPassword != "")
      gServerPassword = gIniSettings.serverPassword;
   // else rely on gServerPassword default of ""

   if(gCmdLineSettings.adminPassword != "")
      gAdminPassword = gCmdLineSettings.adminPassword;
   else if(gIniSettings.adminPassword != "")
      gAdminPassword = gIniSettings.adminPassword;
   // else rely on gAdminPassword default of ""   i.e. no one can do admin tasks on the server

   if(gCmdLineSettings.levelChangePassword != "")
      gLevelChangePassword = gCmdLineSettings.levelChangePassword;
   else if(gIniSettings.levelChangePassword != "")
      gLevelChangePassword = gIniSettings.levelChangePassword;
   // else rely on gLevelChangePassword default of ""   i.e. no one can change levels on the server

  
   // This way, the main level dir can be specified in the INI, but it can either be overridden here,
   // or a subfolder can be specified, depending on what's in the leveldir param
   gConfigDirs.levelDir = getLevelsFolder(gCmdLineSettings.dirs.levelDir != "" ? gCmdLineSettings.dirs.levelDir : gIniSettings.levelDir);
   
   // Other folders can't currently be specified in the INI file
   if(gCmdLineSettings.dirs.rootDataDir != "") 
   {
       gConfigDirs.iniDir = gCmdLineSettings.dirs.rootDataDir;
       gConfigDirs.logDir = gCmdLineSettings.dirs.rootDataDir;
       gConfigDirs.robotDir = joindir(gCmdLineSettings.dirs.rootDataDir, "robots");
       gConfigDirs.screenshotDir = joindir(gCmdLineSettings.dirs.rootDataDir, "screenshots");
       gConfigDirs.levelDir = joindir(gCmdLineSettings.dirs.rootDataDir, "levels");
   }

   if(gIniSettings.levelDir != "")
      gConfigDirs.levelDir = gIniSettings.levelDir;
   else
      gIniSettings.levelDir = gConfigDirs.levelDir;     // So a good default will be written to the INI


   if(gCmdLineSettings.dirs.iniDir != "") gConfigDirs.iniDir = gCmdLineSettings.dirs.iniDir;
   if(gCmdLineSettings.dirs.logDir != "") gConfigDirs.logDir = gCmdLineSettings.dirs.logDir;
   if(gCmdLineSettings.dirs.luaDir != "") gConfigDirs.luaDir = gCmdLineSettings.dirs.luaDir;
   if(gCmdLineSettings.dirs.robotDir != "") gConfigDirs.robotDir = gCmdLineSettings.dirs.robotDir;
   if(gCmdLineSettings.dirs.screenshotDir != "") gConfigDirs.screenshotDir = gCmdLineSettings.dirs.screenshotDir;
   if(gCmdLineSettings.dirs.sfxDir != "") gConfigDirs.sfxDir = gCmdLineSettings.dirs.sfxDir;

   if(gCmdLineSettings.hostname != "")
      gHostName = gCmdLineSettings.hostname;
   else
      gHostName = gIniSettings.hostname;

   if(gCmdLineSettings.hostdescr != "")
      gHostDescr = gCmdLineSettings.hostdescr;
   else
      gHostDescr = gIniSettings.hostdescr;

   if(gCmdLineSettings.hostaddr != "")
      gBindAddress.set(gCmdLineSettings.hostaddr);
   else if(gIniSettings.hostaddr != "")
      gBindAddress.set(gIniSettings.hostaddr);
   // else stick with default defined earlier

   U32 maxplay;
   if (gCmdLineSettings.maxplayers > 0)
      maxplay = gCmdLineSettings.maxplayers;
   else
      maxplay = gIniSettings.maxplayers;
   if (maxplay < 0 || maxplay > 128)
      maxplay = 128;
   gMaxPlayers = (U32) maxplay;


   if(gCmdLineSettings.fullscreen)
      gIniSettings.fullscreen = true;    // Simply clobber the gINISettings copy
   else if(gCmdLineSettings.window)
      gIniSettings.fullscreen = false;

   if(gCmdLineSettings.xpos != -9999)
      gIniSettings.winXPos = gCmdLineSettings.xpos;
   if(gCmdLineSettings.ypos != -9999)
      gIniSettings.winYPos = gCmdLineSettings.ypos;
   if(gCmdLineSettings.winWidth > 0)
      gIniSettings.winSizeFact = max((F32) gCmdLineSettings.winWidth / (F32) UserInterface::canvasWidth, 0.15f);

   if(gCmdLineSettings.masterAddress != "")
      gMasterAddress.set(gCmdLineSettings.masterAddress);
   else
      gMasterAddress.set(gIniSettings.masterAddress);    // This will always have a value

   if(gCmdLineSettings.name != "")                       // We'll clobber the INI file setting.  Since this
      gIniSettings.name = gCmdLineSettings.name;         // setting is never saved, we won't mess up our INI


   // Note that we can be in both clientMode and serverMode (such as when we're hosting a game interactively)

   if(gCmdLineSettings.clientMode)               // Create ClientGame object
      gClientGame = new ClientGame(Address());   //   let the system figure out IP address and assign a port

   //LevelListLoader::buildLevelList();            // Get our level list squared away before we jump off to initHostGame() if that's what's going to happen

   if(gCmdLineSettings.serverMode)
      initHostGame(gBindAddress, false);         // Start hosting
   else if(gCmdLineSettings.connectRemote)       //       or
      joinGame(gConnectAddress, false, false);   // Connect to a game server (i.e. bypass master matchmaking)


   // Not immediately starting a connection...  start out with name entry or main menu
   if(!gCmdLineSettings.connectRemote && !gDedicatedServer)
   {
      if(gIniSettings.name == "")
         gNameEntryUserInterface.activate();
      else
      {
         gMainMenuUserInterface.activate();
         gReadyToConnectToMaster = true;         // Set elsewhere if in dedicated server mode
      }
   }
}


// Any folders not set here default to current folder
void setDefaultConfigDirs()
{
   gConfigDirs.levelDir = "levels";
   gConfigDirs.robotDir = "robots";
   gConfigDirs.screenshotDir = "screenshots";
   gConfigDirs.sfxDir = "sfx";
}


void processCmdLineParams(int argc, char **argv)
{
   Vector<TNL::StringPtr> theArgv;

   // Process some command line args that need to be handled early, like journaling options
   for(S32 i = 1; i < argc; i++)
   {
      if(!stricmp(argv[i], "-rules"))            // Print current rules and exit
      {
         GameType::printRules();
         exitGame(0);
      }
      else if(!stricmp(argv[i], "-jsave"))      // Write game to journal
      {
         if(i != argc - 1)
         {
            gZapJournal.record(argv[i+1]);
            i++;
         }
      }
      else if(!stricmp(argv[i], "-jplay"))      // Replay game from journal
      {
         if(i != argc - 1)
         {
            gZapJournal.load(argv[i+1]);
            i++;
         }
      }
      else
         theArgv.push_back(argv[i]);
   }  // End processing command line args

   gZapJournal.readCmdLineParams(theArgv);   // Process normal command line params, read INI, and start up
}


};  // namespace Zap


using namespace Zap;

////////////////////////////////////////
////////////////////////////////////////
// main()
////////////////////////////////////////
////////////////////////////////////////

#ifdef TNL_OS_XBOX
int zapmain(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
#ifdef TNL_OS_MAC_OSX
   // Move to the application bundle's path (RDW)
   moveToAppPath();
#endif
   setDefaultConfigDirs();
   gCmdLineSettings.init();      // Init cmd line settings struct

   processCmdLineParams(argc, argv);

   string dir = "";
   
   // Go through the following rigamarole because gConfigDirs hasn't been setup yet...
   if(gCmdLineSettings.dirs.iniDir != "")               // Direct specification of ini path takes precedence
       dir = gCmdLineSettings.dirs.iniDir;
   else if(gCmdLineSettings.dirs.rootDataDir != "")     // over specification via rootdatadir param
       dir = gCmdLineSettings.dirs.rootDataDir;

   gINI.SetPath(joindir(dir, "bitfighter.ini"));   
   gIniSettings.init();                      // Init struct that holds INI settings


   gZapJournal.processNextJournalEntry();    // If we're replaying a journal, this will cause the cmd line params to be read from the saved journal

   loadSettingsFromINI();                    // Read INI

   processStartupParams();                   // And merge command line params and INI settings
   SFXObject::init();

   Ship::computeMaxFireDelay();              // Look over weapon info and get some ranges

#ifndef ZAP_DEDICATED
   if(gClientGame)     // That is, we're starting up in interactive mode, as opposed to running a dedicated server
   {
      FXManager::init();                     // Get ready for sparks!!  C'mon baby!!
      InitJoystick();
      resetKeyStates();                      // Reset keyboard state mapping to show no keys depressed
      gAutoDetectedJoystickType = autodetectJoystickType();
      gOptionsMenuUserInterface.setJoystick(gAutoDetectedJoystickType);     // Will override INI settings, so process INI first

      glutInitWindowSize(gScreenWidth, gScreenHeight);                      // Does this actually do anything?  Seem to get same result, regardless of params!
      glutInit(&argc, argv);


      // On OS X, glutInit changes the working directory to the app
      // bundle's resource directory.  We don't want that. (RDW)
#ifdef TNL_OS_MAC_OSX
      moveToAppPath();
#endif
      // InitSdlVideo();      // Get our main SDL rendering window all set up
      // SDL_ShowCursor(0);   // Hide cursor

      glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
      glutCreateWindow(gWindowTitle);

      // Register keyboard/mouse event handlers -- see GLUT docs for details
      glutDisplayFunc(GLUT_CB_display);        // Called when GLUT thinks display needs to be redrawn
      glutReshapeFunc(GLUT_CB_reshape);        // Handle window reshape events
      glutPassiveMotionFunc(GLUT_CB_passivemotion);  // Handle mouse motion when button is not pressed
      glutMotionFunc(GLUT_CB_motion);          // Handle mouse motion when button is pressed
      glutKeyboardFunc(GLUT_CB_keydown);       // Handle key-down events for regular keys
      glutKeyboardUpFunc(GLUT_CB_keyup);       // Handle key-up events for regular keys
      glutSpecialFunc(GLUT_CB_specialkeydown); // Handle key-down events for special keys
      glutSpecialUpFunc(GLUT_CB_specialkeyup); // Handle key-up events for special keys
      glutMouseFunc(GLUT_CB_mouse);            // Handle mouse-clicks

      glutIdleFunc(idle);                      // Register our idle function.  This will get run whenever GLUT is idling
      glutSetCursor(GLUT_CURSOR_NONE);         // Turn off the cursor -- we'll turn this back on in the editor, or when the user tries to use mouse to work menus

      glMatrixMode(GL_PROJECTION);
      glOrtho(0, gScreenWidth, gScreenHeight, 0, 0, 1);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(gScreenWidth/2, gScreenHeight/2, 0);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glLineWidth(gDefaultLineWidth);

      atexit(onExit);
      gOptionsMenuUserInterface.actualizeScreenMode(true);     // Create a display window

      glutMainLoop();         // Launch GLUT on it's merry way.  It'll call back with events and when idling.
      // dedicatedServerLoop();  //    Instead, with SDL, loop forever, running the idle command endlessly

   }
   else                       // We're running a dedicated server so...
#endif
      dedicatedServerLoop();  //    loop forever, running the idle command endlessly

   return 0;
}

