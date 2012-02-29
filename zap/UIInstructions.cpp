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

#include "UIInstructions.h"
#include "UIMenus.h"
#include "UIGame.h"
#include "UIChat.h"
#include "UIDiagnostics.h"
#include "ClientGame.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "teleporter.h"
#include "EngineeredItem.h"
#include "input.h"
#include "speedZone.h"           // For SpeedZone::height
#include "GeomUtils.h"      // For polygon triangulation
#include "config.h"
#include "Colors.h"
#include "ScreenInfo.h"
#include "JoystickRender.h"

#include "SDL/SDL_opengl.h"

namespace Zap
{

// Constructor
InstructionsUserInterface::InstructionsUserInterface(ClientGame *game) : Parent(game)
{
   setMenuID(InstructionsUI);
   
   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   col1 = horizMargin + 0 * canvasWidth / 4;
   col2 = horizMargin + canvasWidth / 4 + 45;     // +45 to make a little more room for Action col
   col3 = horizMargin + canvasWidth / 2;
   col4 = horizMargin + (canvasWidth * 3) / 4 + 45;
}


void InstructionsUserInterface::onActivate()
{
   mCurPage = 1;
}


static const U32 NUM_PAGES = 13;
static const S32 FIRST_COMMAND_PAGE = 9;

static const char *pageHeaders[] = {
   "CONTROLS",
   "LOADOUT SELECTION",
   "MODULES",
   "WEAPON PROJECTILES",
   "SPY BUGS",
   "GAME OBJECTS",
   "MORE GAME OBJECTS",
   "MORE GAME OBJECTS",
   "ADVANCED COMMANDS",
   "LEVEL COMMANDS",
   "ADMIN COMMANDS",
   "DEBUG COMMANDS",
   "SCRIPTING CONSOLE"
};


static ControlStringsEditor consoleCommands1[] = {
   { "add <a> <b>", "Print a + b -- test command" },
   { "exit, quit", "Close the console" },
   { "", "" },      // End of list
};


void InstructionsUserInterface::render()
{
   glColor(Colors::red);
   drawStringf(3, 3, 25, "INSTRUCTIONS - %s", pageHeaders[mCurPage - 1]);
   drawStringf(625, 3, 25, "PAGE %d/%d", mCurPage, NUM_PAGES);
   drawCenteredString(571, 20, "LEFT - previous page  RIGHT, SPACE - next page  ESC exits");

   glColor(0.7f);
   drawHorizLine(0, 800, 31);
   drawHorizLine(0, 800, 569);

   switch(mCurPage)
   {
      case 1:
         renderPage1();
         break;
      case 2:
         renderPage2();
         break;
      case 3:
         renderModulesPage();
         break;
      case 4:
         renderPageObjectDesc(0);
         break;
      case 5:
         renderPageObjectDesc(1);
         break;
      case 6:
         renderPageObjectDesc(2);
         break;
      case 7:
         renderPageObjectDesc(3);
         break;
      case 8:
         renderPageObjectDesc(4);
         break;
      case 9:
         renderPageCommands(0);
         break;
      case 10:
         renderPageCommands(1, "Level change permissions are required to use these commands");     // Level control commands
         break;
      case 11:
         renderPageCommands(2, "Admin permissions are required to use these commands");            // Admin commands
         break;
      case 12:
         renderPageCommands(3);     // Debug commands
         break;
      case 13:
         renderConsoleCommands("Open the console by pressing [Ctrl-/] in game", consoleCommands1);   // Scripting console
         break;

      // When adding page, be sure to increase NUM_PAGES, and add item to pageHeaders array
   }
}


void InstructionsUserInterface::activateInCommandMode()
{
   this->activate();
   mCurPage = FIRST_COMMAND_PAGE;
}


struct ControlString
{
   const char *controlString;
   InputCodeManager::BindingName primaryControlIndex;    // Not really a good name
};

InputCode dummyMouse = MOUSE;
InputCode dummyStickLeft = LEFT_JOYSTICK;
InputCode dummyStickRight = RIGHT_JOYSTICK;
InputCode dummyMoveShipKeysUD = KEYS_UP_DOWN;
InputCode dummyMoveShipKeysLR = KEYS_LEFT_RIGHT;
InputCode dummyMsgMode = KEY_CTRL_M;
InputCode dummySSMode = KEY_CTRL_Q;

static ControlString controlsKeyboard[] = {
         { "Move ship", InputCodeManager::BINDING_DUMMY_MOVE_SHIP_KEYS_UD},
         { " ", InputCodeManager::BINDING_DUMMY_MOVE_SHIP_KEYS_LR},
         { "Aim ship", InputCodeManager::BINDING_DUMMY_MOVE_SHIP_KEYS_MOUSE },
         { "Fire weapon", InputCodeManager::BINDING_FIRE },
         { "Activate module 1", InputCodeManager::BINDING_MOD1 },
         { "Activate module 2", InputCodeManager::BINDING_MOD2 },
         { "-", InputCodeManager::BINDING_NONE },
         { "Open ship config menu", InputCodeManager::BINDING_LOADOUT },
         { "Toggle map view", InputCodeManager::BINDING_CMDRMAP },
         { "Drop flag", InputCodeManager::BINDING_DROPITEM },
         { "Show scoreboard", InputCodeManager::BINDING_SCRBRD },
         { NULL, InputCodeManager::BINDING_NONE },      // End col 1
         { "Cycle current weapon", InputCodeManager::BINDING_ADVWEAP },
         { "Select weapon 1", InputCodeManager::BINDING_SELWEAP1 },
         { "Select weapon 2", InputCodeManager::BINDING_SELWEAP2 },
         { "Select weapon 3", InputCodeManager::BINDING_SELWEAP3 },
         { "-", InputCodeManager::BINDING_NONE },
         { "Open QuickChat menu", InputCodeManager::BINDING_QUICKCHAT },
         { "Chat to team", InputCodeManager::BINDING_TEAMCHAT },
         { "Chat to everyone", InputCodeManager::BINDING_GLOBCHAT },
         { "Record voice chat", InputCodeManager::BINDING_TOGVOICE },
         { "Message display mode", InputCodeManager::BINDING_DUMMY_MSG_MODE },
         { "Save screenshot", InputCodeManager::BINDING_DUMMY_SS_MODE },
         { NULL, InputCodeManager::BINDING_NONE },   // End col 2
      };

static ControlString controlsGamepad[] = {
         { "Move Ship", InputCodeManager::BINDING_DUMMY_STICK_LEFT },
         { "Aim Ship/Fire Weapon", InputCodeManager::BINDING_DUMMY_STICK_RIGHT },
         { "Activate module 1", InputCodeManager::BINDING_MOD1 },
         { "Activate module 2", InputCodeManager::BINDING_MOD2 },
         { "-", InputCodeManager::BINDING_NONE },
         { "Open ship config menu", InputCodeManager::BINDING_LOADOUT },
         { "Toggle map view", InputCodeManager::BINDING_CMDRMAP },
         { "Drop flag", InputCodeManager::BINDING_DROPITEM },
         { "Show scoreboard", InputCodeManager::BINDING_SCRBRD },
         { NULL, InputCodeManager::BINDING_NONE },
         { "Cycle current weapon", InputCodeManager::BINDING_ADVWEAP },
         { "Select weapon 1", InputCodeManager::BINDING_SELWEAP1 },
         { "Select weapon 2", InputCodeManager::BINDING_SELWEAP2 },
         { "Select weapon 3", InputCodeManager::BINDING_SELWEAP3 },
         { "-", InputCodeManager::BINDING_NONE },
         { "Open QuickChat menu", InputCodeManager::BINDING_QUICKCHAT },
         { "Chat to team", InputCodeManager::BINDING_TEAMCHAT },
         { "Chat to everyone", InputCodeManager::BINDING_GLOBCHAT },
         { "Record voice chat", InputCodeManager::BINDING_TOGVOICE },
         { NULL, InputCodeManager::BINDING_NONE },
      };


bool InstructionsUserInterface::usingArrowKeys()
{
   GameSettings *settings = getGame()->getSettings();

   return getInputCode(settings, InputCodeManager::BINDING_LEFT)  == KEY_LEFT  &&
          getInputCode(settings, InputCodeManager::BINDING_RIGHT) == KEY_RIGHT &&
          getInputCode(settings, InputCodeManager::BINDING_UP)    == KEY_UP    &&
          getInputCode(settings, InputCodeManager::BINDING_DOWN)  == KEY_DOWN;
}


static const Color txtColor = Colors::cyan;
static const Color keyColor = Colors::white;     
static const Color secColor = Colors::yellow;

void InstructionsUserInterface::renderKeyBindingQuad(S32 y, const char *str1, InputCodeManager::BindingName binding1, 
                                                            const char *str2, InputCodeManager::BindingName binding2)
{
   GameSettings *settings = getGame()->getSettings();
   
   glColor(txtColor);
   drawString(col1, y, 18, str1);

   if(binding1 != InputCodeManager::BINDING_NONE)
   {
      glColor(keyColor);
      drawStringf(col2, y, 18, "[%s]",  getInputCodeString(settings, binding1));
   }

   glColor(txtColor);
   drawString(col3, y, 18, str2);

   if(binding2 != InputCodeManager::BINDING_NONE)
   {
      glColor(keyColor);
      drawStringf(col4, y, 18, "[%s]",  getInputCodeString(settings, binding2));
   }
}


// This has become rather ugly and inelegant.  Sorry!
void InstructionsUserInterface::renderPage1()
{
   S32 starty = 75;
   S32 y;
   S32 actCol = col1;      // Action column
   S32 contCol = col2;     // Control column

   GameSettings *settings = getGame()->getSettings();

   bool firstCol = true;
   bool done = false;

   drawHorizLine(col1, 750, starty + 26);

   ControlString *controls = (getGame()->getSettings()->getInputCodeManager()->getInputMode() == InputModeKeyboard) ? 
                                              controlsKeyboard : controlsGamepad;

   glColor(secColor);
   drawString(col1, starty, 20, "Action");
   drawString(col2, starty, 20, "Control");
   drawString(col3, starty, 20, "Action");
   drawString(col4, starty, 20, "Control");

   y = starty + 28;

   for(S32 i = 0; !done; i++)              // Iterate over ControlStrings above
   {
      if(!controls[i].controlString)
      {
         if(!firstCol)
            done = true;
         else
         {  // Start second column
            firstCol = false;
            y = starty + 2;
            actCol = col3;
            contCol = col4;

            glColor(secColor);
         }
      }
      else if(!strcmp(controls[i].controlString, "-"))      // Horiz spacer
      {
         glColor(Colors::gray40);
         drawHorizLine(actCol, actCol + 335, y + 13);
      }
      else
      {
         glColor(txtColor);
         drawString(actCol, y, 18, controls[i].controlString);      // Textual description of function (1st arg in lists above)

         glColor(keyColor);

         // We'll also handle special case of arrow keys...
         if(controls[i].primaryControlIndex == InputCodeManager::BINDING_DUMMY_MOVE_SHIP_KEYS_UD)
         {     // (braces needed)
            if(usingArrowKeys())
               drawString(contCol, y, 18, "Arrow Keys");
            else     // Center Up key above Down key
               drawStringf(contCol + getStringWidthf(15, "[%s] ", getInputCodeString(settings, InputCodeManager::BINDING_LEFT)), y + 4, 
                                                     15, "[%s]",  getInputCodeString(settings, InputCodeManager::BINDING_UP));
         }
         else if (controls[i].primaryControlIndex == InputCodeManager::BINDING_DUMMY_MOVE_SHIP_KEYS_LR)
         {     // (braces needed)
            if(usingArrowKeys())
               y -= 26;    // Hide this line
            else
               drawStringf(col2, y + 4, 15, "[%s] [%s] [%s]", 
                     getInputCodeString(settings, InputCodeManager::BINDING_LEFT),
                     getInputCodeString(settings, InputCodeManager::BINDING_DOWN),
                     getInputCodeString(settings, InputCodeManager::BINDING_RIGHT));
         }
         else
            JoystickRender::renderControllerButton((F32)contCol, F32(y + 4), Joystick::SelectedPresetIndex,
                                                   getInputCode(settings, controls[i].primaryControlIndex), false);
      }

      y += 26;
   }

   y += 10;
   glColor(secColor);
   drawCenteredString(y, 20, "These special keys are also usually active:");

   y += 40;
   renderKeyBindingQuad(y, "Help",    InputCodeManager::BINDING_HELP,    "Universal Chat",               InputCodeManager::BINDING_OUTGAMECHAT);
   y += 26;
   renderKeyBindingQuad(y, "Mission", InputCodeManager::BINDING_MISSION, "Display FPS / Lag",            InputCodeManager::BINDING_FPS);
   y += 26;
   renderKeyBindingQuad(y, "",        InputCodeManager::BINDING_NONE,    "Diagnostics", InputCodeManager::BINDING_DIAG);
}


static const char *loadoutInstructions1[] = {
   "LOADOUTS",
   "Players can outfit their ships with 3 weapons and 2 modules.",      // TODO: Replace 3 & 2 w/constants
   "Pressing the ship configuration menu key brings up a menu that",
   "allows the player to choose the next loadout for his or her ship.",
   "",
   "This loadout will become active on the ship when the player",
   "flies over a Loadout Zone area, or respawns on a level that",
   "has no Loadout Zones.",
};

static const char *loadoutInstructions2[] = {
   "PRESETS",
   "You can save your Loadout in a Preset for easy recall later.",
   "To save your loadout, press [Ctrl-1], [Ctrl-2], or [Ctrl-3].",
   "To recall the preset, press [Alt-1], [Alt-2], or [Alt-3].",
   "",
   "Loadout Presets will be saved when you quit the game, and",
   "will be available the next time you play."
};

void InstructionsUserInterface::renderPage2()
{
   S32 y = 45;
   S32 gap = 40;

   glColor(Colors::yellow);
   for(U32 i = 0; i < ARRAYSIZE(loadoutInstructions1); i++)
   {
      drawCenteredString(y, 20, loadoutInstructions1[i]);
      y += gap;
      glColor(Colors::white);
      gap = 26;
   }

   y += 40;
   gap = 40;

   glColor(Colors::yellow);
   for(U32 i = 0; i < ARRAYSIZE(loadoutInstructions2); i++)
   {
      drawCenteredString(y, 20, loadoutInstructions2[i]);
      y += gap;
      glColor(Colors::cyan);
      gap = 26;
   }
}


static const char *moduleInstructions[] = {
   "Modules have up to 3 modes: Passive, Active, and Kinetic (P/A/K)",
   "Passive mode is always active and costs no energy (e.g. Armor).",
   "Use Active mode by pressing module's activation key (e.g. Shield).",
   "Double-click activation key to use module's Kinetic mode.",
};

static const char *moduleDescriptions[][2] = {
   { "Boost: ",    "Turbo (A), Super (K)" },
   { "Shield: ",   "Reflects incoming projectiles (A)" },
   { "Armor: ",    "Reduces damage, makes ship harder to control (P)" },
   { "",           "Incoming bouncers do more damage" },
   { "Repair: ",   "Repair self and nearby damaged objects (A)" },
   { "Sensor: ",   "See further (P), Reveal hidden objects (A)," },
   { "",           "Deploy spy bugs (K)" },
   { "Cloak: ",    "Make ship invisible to enemies (A)" },
   { "Engineer: ", "Collect resources to build turrets and forcefields (A)" }
};

void InstructionsUserInterface::renderModulesPage()
{
   S32 y = 40;
   S32 textsize = 20;

   glColor(Colors::white);

   for(U32 i = 0; i < ARRAYSIZE(moduleInstructions); i++)
   {
      drawCenteredString(y, textsize, moduleInstructions[i]);
      y += 26;
   }

   y += 20;

   glColor(Colors::cyan);
   drawCenteredString(y, textsize, "LIST OF MODULES");

   y += 35;


   for(U32 i = 0; i < ARRAYSIZE(moduleDescriptions); i++)
   {
      S32 x = 105;
      glColor(Colors::yellow);
      x += drawStringAndGetWidth(x, y, textsize, moduleDescriptions[i][0]);

      // Hacky special case  TODO: find a way to generalize this
      if(i == 3 || i == 6)
   {
         x += getStringWidth(textsize, moduleDescriptions[i - 1][0]);
         y -= 20;
      }

      glColor(Colors::white);
      drawString(x, y, textsize, moduleDescriptions[i][1]);

      glPushMatrix();
      glTranslatef(60, F32(y + 10), 0);
      glScale(0.7f);
      glRotatef(-90, 0, 0, 1);

      static F32 thrusts[4] =  { 1, 0, 0, 0 };
      static F32 thrustsBoost[4] =  { 1.3f, 0, 0, 0 };

      switch(i)
      {
         case 0:     // Boost
            renderShip(&Colors::blue, 1, thrustsBoost, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false, false);
            glBegin(GL_LINES);
               glColor(Colors::yellow);
               glVertex2f(-20, -17);
               glColor(Colors::black);
               glVertex2f(-20, -50);
               glColor(Colors::yellow);
               glVertex2f(20, -17);
               glColor(Colors::black);
               glVertex2f(20, -50);
            glEnd();
            break;

         case 1:     // Shield
            renderShip(&Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, true, false, false, false);
            break;

         case 2:     // Armor
            renderShip(&Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false, true);
            break;

         // skip 3 for 2nd line of armor

         case 4:     // Repair
            {
               F32 health = (getGame()->getCurrentTime() & 0x7FF) * 0.0005f;

               F32 alpha = 1.0;
               renderShip(&Colors::blue, alpha, thrusts, health, (F32)Ship::CollisionRadius, 0, false, false, false, true, false);
            }
            break;

         case 5:     // Sensor
            renderShip(&Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, getGame()->getCurrentTime(), false, false, true, false, false);
            break;

         // skip 6 for 2nd line of sensor

         case 7: // Cloak
            {
               U32 time = getGame()->getCurrentTime();
               F32 frac = F32(time & 0x3FF);
               F32 alpha;
               if((time & 0x400) != 0)
                  alpha = frac * 0.001f;
               else
                  alpha = 1 - (frac * 0.001f);
               renderShip(&Colors::blue, alpha, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false, false);
            }
            break;

         case 8:     // Engineer
            {
               renderShip(&Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false, false);
               renderResourceItem(Point(0,0));
            }
            break;
      }
      glPopMatrix();
      y += 45;
   }
}

const char *gGameObjectInfo[] = {
   /* 00 */   "Phaser", "The default weapon",
   /* 01 */   "Bouncer", "Bounces off walls",
   /* 03 */   "Triple", "Fires three diverging shots",
   /* 03 */   "Burst", "Explosive projectile",
   /* 04 */   "Friendly Mine", "Team's mines show trigger radius",
   /* 05 */   "Enemy Mine", "These are much harder to see",

   /* 06 */   "Friendly Spy Bug", "Lets you surveil the area",
   /* 07 */   "Enemy Spy Bug", "Destroy these when you find them",
   /* 08 */   "", "",
   /* 09 */   "", "",
   /* 10 */   "", "",
   /* 11 */   "", "",

   /* 12 */   "Repair Item", "Repairs damage to ship",
   /* 13 */   "Energy Item", "Restores ship's energy",
   /* 14 */   "Neutral Turret", "Repair to take team ownership",
   /* 15 */   "Active Turret", "Fires at enemy team",
   /* 16 */   "Neutral Emitter", "Repair to take team ownership",
   /* 17 */   "Force Field Emitter", "Allows only one team to pass",

   /* 18 */   "Teleporter", "Warps ship to another location",
   /* 19 */   "Flag", "Objective item in some game types",
   /* 20 */   "Loadout Zone", "Updates ship configuration",
   /* 21 */   "Nexus", "Bring flags here in Nexus game",
   /* 22 */   "Asteroid", "Silent but deadly",
   /* 23 */   "GoFast", "Makes ship go fast",

   /* 24 */   "Test Item", "Bouncy ball",
   /* 25 */   "Resource Item", "Use with engineer module",
   /* 26 */   "Soccer Ball", "Push into enemy goal in Soccer game",
   /* 27 */   "Core", "Kill the enemy's; defend yours OR DIE!"
};

static U32 GameObjectCount = ARRAYSIZE(gGameObjectInfo) / 2;   


void InstructionsUserInterface::renderPageObjectDesc(U32 index)
{
   U32 objectsPerPage = 6;
   U32 startIndex = index * objectsPerPage;
   U32 endIndex = startIndex + objectsPerPage;
   if(endIndex > GameObjectCount)
      endIndex = GameObjectCount;

   for(U32 i = startIndex; i < endIndex; i++)
   {
      const char *text = gGameObjectInfo[i * 2];
      const char *desc = gGameObjectInfo[i * 2 + 1];
      S32 index = i - startIndex;

      Point objStart((index & 1) * 400, (index >> 1) * 165);
      objStart += Point(200, 90);
      Point start = objStart + Point(0, 55);

      glColor(Colors::yellow);
      renderCenteredString(start, (S32)20, text);

      glColor(Colors::white);
      renderCenteredString(start + Point(0, 25), (S32)17, desc);

      glPushMatrix();
      glTranslate(objStart);
      glScale(0.7f);


      // TODO: Do this once, elsewhere
      Vector<Point> speedZoneRenderPoints;
      SpeedZone::generatePoints(Point(-SpeedZone::height / 2, 0), Point(1, 0), 1, speedZoneRenderPoints);

      switch(i)
      {
         case 0:
            renderProjectile(Point(0, 0), 0, getGame()->getCurrentTime());
            break;
         case 1:
            renderProjectile(Point(0, 0), 1, getGame()->getCurrentTime());
            break;
         case 2:
            renderProjectile(Point(0, 0), 2, getGame()->getCurrentTime());
            break;
         case 3:
            renderGrenade(Point(0, 0), 1, getGame()->getSettings()->getIniSettings()->burstGraphicsMode);
            break;
         case 4:
            renderMine(Point(0, 0), true, true);
            break;
         case 5:
            renderMine(Point(0, 0), true, false);
            break;
         case 6:
            renderSpyBug(Point(0, 0), Colors::blue, true, true);
            break;
         case 7:
            renderSpyBug(Point(0, 0), Colors::blue, false, true);
            break;
         case 8:     // Blank
         case 9:
         case 10:
         case 11:
            break;
         case 12:
            renderRepairItem(Point(0, 0));
            break;
         case 13:
            renderEnergyItem(Point(0, 0));
            break;
         case 14:
            renderTurret(Colors::white, Point(0, 15), Point(0, -1), false, 0, 0);
            break;
         case 15:
            renderTurret(Colors::blue, Point(0, 15), Point(0, -1), true, 1, 0);
            break;

         case 16:
            renderForceFieldProjector(Point(-7.5, 0), Point(1, 0), &Colors::white, false);
            break;
         case 17:
            renderForceFieldProjector(Point(-50, 0), Point(1, 0), &Colors::red, true);
            renderForceField(Point(-35, 0), Point(50, 0), &Colors::red, true);
            break;
         case 18:
            renderTeleporter(Point(0, 0), 0, true, getGame()->getCurrentTime(), 1, 1, (F32)Teleporter::TELEPORTER_RADIUS, 1, Vector<Point>(), false);
            break;
         case 19:
            renderFlag(0, 0, &Colors::red);
            break;
         case 20:    // Loadout zone
            {              // braces needed: see C2360
               Vector<Point> o;     // outline
               o.push_back(Point(-150, -30));
               o.push_back(Point(150, -30));
               o.push_back(Point(150, 30));
               o.push_back(Point(-150, 30));

               Vector<Point> f;     // fill
               Triangulate::Process(o, f);

               renderLoadoutZone(&Colors::blue, &o, &f, findCentroid(o), angleOfLongestSide(o));
            }

            break;

         case 21:    // Nexus
            {
               Vector<Point> o;     // outline
               o.push_back(Point(-150, -30));
               o.push_back(Point(150, -30));
               o.push_back(Point(150, 30));
               o.push_back(Point(-150, 30));

               Vector<Point> f;     // fill
               Triangulate::Process(o, f);

               renderNexus(&o, &f, findCentroid(o), angleOfLongestSide(o), 
                                       getGame()->getCurrentTime() % 5000 > 2500, 0);
            }
            break;

         case 22:    // Asteroid... using goofball factor to keep out of sync with Nexus graphic
            renderAsteroid(Point(0,-10), 
                     (S32)(getGame()->getCurrentTime() / 2891) % Asteroid::getDesignCount(), .7f);    
            break;

         case 23:    // SpeedZone
            renderSpeedZone(&speedZoneRenderPoints, getGame()->getCurrentTime());
            break;

         case 24:    // TestItem
            renderTestItem(Point(0,0));
            break;

         case 25:    // ResourceItem
            renderResourceItem(Point(0,0));
            break;

         case 26:    // SoccerBall
            renderSoccerBall(Point(0,0));
            break;

         case 27:    // Core
            F32 health[] = { 1,1,1,1,1,1,1,1,1,1 };
            renderCore(Point(0,0), 55, &Colors::blue, Platform::getRealMilliseconds(), health, 1);
            break;
      }
      glPopMatrix();
      objStart.y += 75;
      start.y += 75;
   }
}


extern CommandInfo chatCmds[];
extern S32 chatCmdSize;

void InstructionsUserInterface::renderPageCommands(U32 page, const char *msg)
{
   TNLAssert(page < COMMAND_CATEGORIES, "Page too high!");

   S32 ypos = 50;

   S32 cmdCol = horizMargin;                                                         // Action column
   S32 descrCol = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 55;   // Control column

   const S32 instrSize = 18;

   glColor(Colors::green);
   drawStringf(cmdCol, ypos, instrSize, "Enter a cmd by pressing [%s], or by typing one at the chat prompt", 
                                        getInputCodeString(getGame()->getSettings(), InputCodeManager::BINDING_CMDCHAT));
   ypos += 28;
   drawString(cmdCol, ypos, instrSize, "Use [TAB] to expand a partially typed command");
   ypos += 28;

   if(strcmp(msg, ""))
   {
      drawString(cmdCol, ypos, instrSize, msg);
      ypos += 28;
   }

   Color cmdColor =   Colors::cyan;
   Color descrColor = Colors::white;
   Color secColor =   Colors::yellow;

   const S32 headerSize = 20;
   const S32 cmdSize = 16;
   const S32 cmdGap = 10;

   glColor(secColor);
   drawString(cmdCol, ypos, headerSize, "Command");
   drawString(descrCol, ypos, headerSize, "Description");

   //glColor3f(0,1,0);
   ypos += cmdSize + cmdGap;
   glBegin(GL_LINES);
      glVertex2i(cmdCol, ypos);
      glVertex2i(750, ypos);
   glEnd();

   ypos += 5;     // Small gap before cmds start


   bool first = true;
   S32 section = -1;

   for(S32 i = 0; i < chatCmdSize && U32(chatCmds[i].helpCategory) <= page; i++)
   {
      if(U32(chatCmds[i].helpCategory) < page)
         continue;

      if(first)
      {
         section = chatCmds[i].helpGroup;
         first = false;
      }

      // Check if we've just changed sections... if so, draw a horizontal line ----------
      if(chatCmds[i].helpGroup > section)      
      {
         glColor(Colors::gray40);

         drawHorizLine(cmdCol, cmdCol + 335, ypos + (cmdSize + cmdGap) / 3);

         section = chatCmds[i].helpGroup;

         ypos += cmdSize + cmdGap;
      }

      glColor(cmdColor);
      
      // Assemble command & args from data in the chatCmds struct
      string cmdString = "/" + chatCmds[i].cmdName;

      for(S32 j = 0; j < chatCmds[i].cmdArgCount; j++)
         cmdString += " " + chatCmds[i].helpArgString[j];

      if(chatCmds[i].lines == 1)    // Everything on one line, the normal case
      {
         drawString(cmdCol, ypos, cmdSize, cmdString.c_str());      
         glColor(descrColor);
         drawString(descrCol, ypos, cmdSize, chatCmds[i].helpTextString.c_str());
      }
      else                          // Draw the command on one line, explanation on the next, with a bit of indent
      {
         drawString(cmdCol, ypos, cmdSize, cmdString.c_str());
         ypos += cmdSize + cmdGap;
         glColor(descrColor);
         drawString(cmdCol + 50, ypos, cmdSize, chatCmds[i].helpTextString.c_str());
      }

      ypos += cmdSize + cmdGap;
   }
}


void InstructionsUserInterface::nextPage()
{
   mCurPage++;

   if(mCurPage > NUM_PAGES)
      mCurPage = 1;
}


void InstructionsUserInterface::prevPage()
{
   mCurPage--;

   if(mCurPage < 1)
      mCurPage = NUM_PAGES;
}


void InstructionsUserInterface::exitInstructions()
{
   playBoop();
   getUIManager()->reactivatePrevUI();      //mGameUserInterface
}


bool InstructionsUserInterface::onKeyDown(InputCode inputCode, char ascii)
{
   if(Parent::onKeyDown(inputCode, ascii)) { /* Do nothing */ }

   else if(inputCode == KEY_LEFT || inputCode == BUTTON_DPAD_LEFT || inputCode == BUTTON_DPAD_UP || inputCode == KEY_UP)
   {
      playBoop();
      prevPage();
   }
   else if(inputCode == KEY_RIGHT        || inputCode == KEY_SPACE || inputCode == BUTTON_DPAD_RIGHT ||
           inputCode == BUTTON_DPAD_DOWN || inputCode == KEY_ENTER || inputCode == KEY_DOWN)
   {
      playBoop();
      nextPage();
   }
   else if(checkInputCode(getGame()->getSettings(), InputCodeManager::BINDING_HELP, inputCode) || 
           inputCode == KEY_ESCAPE  || inputCode == BUTTON_BACK)
      exitInstructions();

   return true;
}

};
