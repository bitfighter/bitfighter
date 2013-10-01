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

#include "UIManager.h"

#include "ClientGame.h"
#include "gameObjectRender.h"
#include "teleporter.h"
#include "speedZone.h"        // For SpeedZone::height
#include "Colors.h"
#include "ScreenInfo.h"
#include "Joystick.h"
#include "JoystickRender.h"
#include "CoreGame.h"         // For coreItem rendering
#include "ChatCommands.h"
#include "ChatHelper.h"       // For ChatHelper::chatCmdSize
#include "FontManager.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"
#include "GeomUtils.h"

#include "tnlAssert.h"


namespace Zap
{


// This must be kept aligned with enum IntructionPages -- TODO put this into xmacro
static const char *pageHeaders[] = {
   "CONTROLS",
   "LOADOUT SELECTION",
   "MODULES",
   "WEAPON PROJECTILES",
   "MINES & SPY BUGS",
   "GAME OBJECTS",
   "MORE GAME OBJECTS",
   "MORE GAME OBJECTS",
   "GAME INDICATORS",
   "ADVANCED COMMANDS",
   "SOUND AND MUSIC",
   "LEVEL COMMANDS",
   "ADMIN COMMANDS",
   "OWNER COMMANDS",
   "DEBUG COMMANDS",
   //"SCRIPTING CONSOLE"

#ifdef TNL_DEBUG
   "TEST COMMANDS"
#endif
};


//static ControlStringsEditor consoleCommands1[] = {
//   { "a = Asteroid.new()", "Create an asteroid and set it to the variable 'a'" },
//   { "a:setVel(100, 0)",   "Set the asteroid's velocity to move slowly to the right" },
//   { "a:addToGame()",      "Insert the asteroid into the currently running game" },
//   { "", "" },      // End of list
//};


// Constructor
InstructionsUserInterface::InstructionsUserInterface(ClientGame *game) : Parent(game),
                                                                         mLoadoutInstructions(LineGap),
                                                                         mPageHeaders(LineGap)
{
   // Quick sanity check...
   TNLAssert(ARRAYSIZE(pageHeaders) == InstructionMaxPages, "pageHeaders not aligned with enum IntructionPages!!!");

   S32 canvasWidth = gScreenInfo.getGameCanvasWidth();

   col1 = horizMargin + 0 * canvasWidth / 4;
   col2 = horizMargin + canvasWidth / 4 + 45;     // +45 to make a little more room for Action col
   col3 = horizMargin + canvasWidth / 2;
   col4 = horizMargin + (canvasWidth * 3) / 4 + 45;

   calcPolygonVerts(Point(0,0), 7, (F32)TestItem::TEST_ITEM_RADIUS, 0, mTestItemPoints);
   ResourceItem::generateOutlinePoints(Point(0,0), 1.0, mResourceItemPoints);


   // Prepare special instructions
   const ControlStringsEditor helpBindLeft[] = 
   { 
      { "Help",              "[[Help]]"    },
      { "Mission",           "[[Mission]]" }
   };

   pack(mSpecialKeysInstrLeft,  mSpecialKeysBindingsLeft, 
        helpBindLeft, ARRAYSIZE(helpBindLeft), getGame()->getSettings());


   const ControlStringsEditor helpBindRight[] = 
   { 
      { "Universal Chat",    "[[OutOfGameChat]]" },
      { "Display FPS / Lag", "[[FPS]]"           },
      { "Diagnostics",       "[[Diagnostics]]"   }
   };

   pack(mSpecialKeysInstrRight, mSpecialKeysBindingsRight, 
        helpBindRight, ARRAYSIZE(helpBindRight), getGame()->getSettings());
}


// Destructor
InstructionsUserInterface::~InstructionsUserInterface()
{
   // Do nothing
}


struct ControlString
{
   const char *controlDescr;
   InputCodeManager::BindingName primaryControlIndex;    // Not really a good name
};


void InstructionsUserInterface::onActivate()
{
   mCurPage = 0;
   mUsingArrowKeys = usingArrowKeys();

   Vector<SymbolShapePtr> symbols;

   GameSettings *settings = getGame()->getSettings();

   initNormalKeys_page1();
   initPage2();
   initPageHeaders();
}


static const S32 FIRST_COMMAND_PAGE = InstructionsUserInterface::InstructionAdvancedCommands;
static const S32 FIRST_OBJ_PAGE     = InstructionsUserInterface::InstructionWeaponProjectiles;

static const Color *txtColor = &Colors::cyan;
static const Color *keyColor = &Colors::white;     
static const Color *secColor = &Colors::yellow;

// Initialize the special keys section of the first page of help
void InstructionsUserInterface::initNormalKeys_page1()
{
   // Needs to be here so if user changes their bindings, we'll see the new ones when we reload!

   ControlStringsEditor controlsKeyboardLeft[] = 
   {
         { "Move ship",             "[[ShipUp]]"          },
         { " ",                     "[[MOVEMENT_LDR]]"    },
         { "Aim ship",              "[[Mouse]]"           },
         { "Fire weapon",           "[[Fire]]"            },
         { "Activate module 1",     "[[ActivateModule1]]" },
         { "Activate module 2",     "[[ActivateModule2]]" },
         { "-",                     ""                    },
         { "Open ship config menu", "[[ShowLoadoutMenu]]" },
         { "Toggle map view",       "[[ShowCmdrMap]]"     },
         { "Drop flag",             "[[DropItem]]"        },
         { "Show scoreboard",       "[[ShowScoreboard]]"  }
   };

   ControlStringsEditor controlsGamepadLeft[] = 
   {
         { "Move Ship",             "Left Joystick"   },
         { "Aim Ship/Fire Weapon",  "Right Joystick"  },
         { "Activate module 1",     "[[ActivateModule1]]" },
         { "Activate module 2",     "[[ActivateModule2]]" },
         { "-",                     ""                    },
         { "Open ship config menu", "[[ShowLoadoutMenu]]" },
         { "Toggle map view",       "[[ShowCmdrMap]]"     },
         { "Drop flag",             "[[DropItem]]"        },
         { "Show scoreboard",       "[[ShowScoreboard]]"  }
   };

   // These controls will work for both joystick and keyboard users
   ControlStringsEditor controlsRight[] = 
   {
         { "Cycle current weapon", "[[SelNextWeapon]]" },
         { "Select weapon 1",      "[[SelWeapon1]]"    },
         { "Select weapon 2",      "[[SelWeapon2]]"    },
         { "Select weapon 3",      "[[SelWeapon3]]"    },
         { "-",                    ""                  },
         { "Chat to everyone",     "[[GlobalChat]]"    },
         { "Chat to team",         "[[TeamChat]]"      },
         { "Open QuickChat menu",  "[[QuickChat]]"     },
         { "Record voice chat",    "[[VoiceChat]]"     },
         { "Message display mode", "[[Ctrl+M]]"        },
         { "Save screenshot",      "[[Ctrl+Q]]"        }
   };


   ControlStringsEditor *helpBindLeft,     *helpBindRight;
   S32                  helpBindLeftCount, helpBindRightCount;

   if(getGame()->getInputMode() == InputModeKeyboard)
   {
      helpBindLeft      = controlsKeyboardLeft;
      helpBindLeftCount = ARRAYSIZE(controlsKeyboardLeft);
   }
   else
   {
      helpBindLeft      = controlsGamepadLeft;
      helpBindLeftCount = ARRAYSIZE(controlsGamepadLeft);
   }

   helpBindRight      = controlsRight;
   helpBindRightCount = ARRAYSIZE(controlsRight);


   UI::SymbolStringSet keysInstrLeft(LineGap),  keysBindingsLeft(LineGap), 
                       keysInstrRight(LineGap), keysBindingsRight(LineGap);

   Vector<SymbolShapePtr> symbols;

   // Add some headers to our 4 columns
   symbols.clear();
   symbols.push_back(SymbolString::getSymbolText("Action", HeaderFontSize, HelpContext, secColor));
   keysInstrLeft.add(SymbolString(symbols));
   keysInstrRight.add(SymbolString(symbols));

   symbols.clear();
   symbols.push_back(SymbolString::getSymbolText("Control", HeaderFontSize, HelpContext, secColor));
   keysBindingsLeft.add(SymbolString(symbols));
   keysBindingsRight.add(SymbolString(symbols));

   // Add horizontal line to first column (will draw across all)
   symbols.clear();
   symbols.push_back(SymbolString::getHorizLine(735, -14, 8, &Colors::gray70));
   keysInstrLeft.add(SymbolString(symbols));

   // Need to add a blank symbol to keep columns in sync
   symbols.clear();
   symbols.push_back(SymbolString::getBlankSymbol(0,0));
   keysBindingsLeft.add(SymbolString(symbols));
   keysInstrRight.add(SymbolString(symbols));
   keysBindingsRight.add(SymbolString(symbols));

   pack(keysInstrLeft,  keysBindingsLeft, 
        helpBindLeft, helpBindLeftCount, getGame()->getSettings());

   pack(keysInstrRight, keysBindingsRight, 
        helpBindRight, helpBindRightCount, getGame()->getSettings());


   S32 centeringOffset = getStringWidth(HelpContext, HeaderFontSize, "Control") / 2;  //(= 33)

   mSymbolSets.clear();

   mSymbolSets.addSymbolStringSet(keysInstrLeft,     AlignmentLeft,   col1);
   mSymbolSets.addSymbolStringSet(keysBindingsLeft,  AlignmentCenter, col2 + centeringOffset);
   mSymbolSets.addSymbolStringSet(keysInstrRight,    AlignmentLeft,   col3);
   mSymbolSets.addSymbolStringSet(keysBindingsRight, AlignmentCenter, col4 + centeringOffset);
}


void InstructionsUserInterface::render()
{
   FontManager::pushFontContext(HelpContext);

   Parent::render(pageHeaders[mCurPage], mCurPage + 1, InstructionMaxPages);          // We +1 to be natural

   switch(mCurPage)
   {
      case InstructionControls:
         renderPage1();
         break;
      case InstructionLoadout:
         renderPage2();
         break;
      case InstructionModules:
         renderModulesPage();
         break;
      case InstructionWeaponProjectiles:
         renderPageObjectDesc(InstructionWeaponProjectiles - FIRST_OBJ_PAGE);
         break;
      case InstructionSpyBugs:
         renderPageObjectDesc(InstructionSpyBugs - FIRST_OBJ_PAGE);
         break;
      case InstructionGameObjects1:
         renderPageObjectDesc(InstructionGameObjects1 - FIRST_OBJ_PAGE);
         break;
      case InstructionGameObjects2:
         renderPageObjectDesc(InstructionGameObjects2 - FIRST_OBJ_PAGE);
         break;
      case InstructionGameObjects3:
         renderPageObjectDesc(InstructionGameObjects3 - FIRST_OBJ_PAGE);
         break;
      case InstructionGameIndicators:
         renderPageGameIndicators();
         break;
      case InstructionAdvancedCommands:
         renderPageCommands(InstructionAdvancedCommands - FIRST_COMMAND_PAGE, 
                            "Tip: Define QuickChat items to quickly enter commands (see INI for details)");
         break;
      case InstructionSoundCommands:
         renderPageCommands(InstructionSoundCommands - FIRST_COMMAND_PAGE);            // Sound control commands
         break;
      case InstructionLevelCommands:
         renderPageCommands(InstructionLevelCommands - FIRST_COMMAND_PAGE, 
                            "Level change permissions are required to use these commands");  
         break;
      case InstructionAdminCommands:
         renderPageCommands(InstructionAdminCommands - FIRST_COMMAND_PAGE, 
                            "Admin permissions are required to use these commands");   // Admin commands
         break;                                                                        
      case InstructionOwnerCommands:                                                   
         renderPageCommands(InstructionOwnerCommands - FIRST_COMMAND_PAGE,             
                            "Owner permissions are required to use these commands");   // Owner commands
         break;                                                                        
      case InstructionDebugCommands:                                                   
         renderPageCommands(InstructionDebugCommands - FIRST_COMMAND_PAGE);            // Debug commands
         break;

#ifdef TNL_DEBUG
      case InstructionTestCommands:
         renderPageCommands(InstructionTestCommands  - FIRST_COMMAND_PAGE,
                            "These commands only available in debug builds");          // Debug commands -- debug builds only
         break;
#endif

      //case InstructionScriptingConsole:
      //   renderConsoleCommands("Open the console by pressing [Ctrl-/] in game", consoleCommands1);   // Scripting console
      //   break;

      // When adding page, be sure to add item to pageHeaders array and InstructionPages enum

      default:
         TNLAssert(false, "Invalid value for mCurPage!");
   }

   FontManager::popFontContext();
}


void InstructionsUserInterface::activatePage(IntructionPages pageIndex)
{
   getUIManager()->activate(this);                // Activates ourselves, essentially
   mCurPage = pageIndex;
}


bool InstructionsUserInterface::usingArrowKeys()
{
   GameSettings *settings = getGame()->getSettings();

   return getInputCode(settings, InputCodeManager::BINDING_LEFT)  == KEY_LEFT  &&
          getInputCode(settings, InputCodeManager::BINDING_RIGHT) == KEY_RIGHT &&
          getInputCode(settings, InputCodeManager::BINDING_UP)    == KEY_UP    &&
          getInputCode(settings, InputCodeManager::BINDING_DOWN)  == KEY_DOWN;
}


// This has become rather ugly and inelegant.  Sorry!
void InstructionsUserInterface::renderPage1()
{
   S32 starty = 65;
   S32 y;
   S32 actCol = col1;      // Action column
   S32 contCol = col2;     // Control column

   GameSettings *settings = getGame()->getSettings();

   bool firstCol = true;
   bool done = false;

   y = starty;

   y += mSymbolSets.render(y);

   y += 35;
   glColor(secColor);
   drawCenteredString_fixed(y, 20, "These special keys are also usually active:");

   y += 36;
   mSpecialKeysInstrLeft.render (col1, y, AlignmentLeft);
   mSpecialKeysInstrRight.render(col3, y, AlignmentLeft);

   S32 centeringOffset = getStringWidth(HelpContext, HeaderFontSize, "Control") / 2;

   mSpecialKeysBindingsLeft.render (col2 + centeringOffset, y, AlignmentCenter);
   mSpecialKeysBindingsRight.render(col4 + centeringOffset, y, AlignmentCenter);
}


static const char *loadoutInstructions1[] = {
   "LOADOUTS",
   "Outfit your ship with 3 weapons and 2 modules.  Press [[ShowLoadoutMenu]] to",      // TODO: Replace 3 & 2 w/constants
   "choose a new loadout for your ship.",
   "",
   "This loadout will become active when you enter a Loadout Zone ([[LOADOUT_ICON]]),",
   "or respawn on a level that has no Loadout Zones.",
};

static const char *loadoutInstructions2[] = {
   "PRESETS",
   "You can save your Loadout in a Preset for easy recall later.",
   "To save your loadout, press [[SaveLoadoutPreset1]], [[SaveLoadoutPreset2]], or [[SaveLoadoutPreset3]].",
   "To recall the preset, press [[LoadLoadoutPreset1]], [[LoadLoadoutPreset2]], or [[LoadLoadoutPreset3]].",
   "",
   "Loadout Presets will be saved when you quit the game, and",
   "will be available the next time you play."
};


// Converts the blocks of text above into SymbolStrings for nicer rendering
static void initPage2Block(const char **block, S32 blockSize, S32 fontSize, const Color *headerColor, const Color *bodyColor, 
                           const InputCodeManager *inputCodeManager, UI::SymbolStringSet &instrBlock)
{
   Vector<SymbolShapePtr> symbols;     

   for(S32 i = 0; i < blockSize; i++)
   {
      if(i == 0)  // First line is a little different than the rest
      {
         symbols.clear();
         symbols.push_back(SymbolString::getSymbolText(block[i], fontSize, HelpContext, headerColor));
         instrBlock.add(SymbolString(symbols, AlignmentCenter));

         // Provide a gap between header and body... when this is rendered, a gap equivalent to a line of text will be shown
         symbols.clear();
         symbols.push_back(SymbolString::getBlankSymbol());
         instrBlock.add(SymbolString(symbols));
      }
      else
      {
         symbols.clear();
         string str(block[i]);
         SymbolString::symbolParse(inputCodeManager, str, symbols, HelpContext, fontSize, bodyColor, keyColor);

         instrBlock.add(SymbolString(symbols, AlignmentLeft));
      }
   }
}


void InstructionsUserInterface::initPage2()
{
   Vector<SymbolShapePtr> symbols;

   mLoadoutInstructions.clear();

   initPage2Block(loadoutInstructions1, ARRAYSIZE(loadoutInstructions1), HeaderFontSize, &Colors::yellow, &Colors::paleGreen,
                  getGame()->getSettings()->getInputCodeManager(), mLoadoutInstructions);

   // Add some space separating the two sections
   symbols.push_back(SymbolString::getBlankSymbol(0, 30));
   mLoadoutInstructions.add(SymbolString(symbols));

   initPage2Block(loadoutInstructions2, ARRAYSIZE(loadoutInstructions2), HeaderFontSize, &Colors::yellow, &Colors::cyan, 
               getGame()->getSettings()->getInputCodeManager(), mLoadoutInstructions);
}


void InstructionsUserInterface::initPageHeaders()
{
   InputCodeManager *inputCodeManager = getGame()->getSettings()->getInputCodeManager();

   Vector<SymbolShapePtr> symbols;

   symbols.clear();
   SymbolString::symbolParse(inputCodeManager, "Use [[Tab]] to expand a partially typed command", 
                             symbols, HelpContext, FontSize, &Colors::green, &Colors::white);
   mPageHeaders.add(SymbolString(symbols, AlignmentLeft));

   symbols.clear();
   SymbolString::symbolParse(inputCodeManager, "Enter a cmd by pressing [[Command]], or by typing one at the chat prompt", 
                             symbols, HelpContext, FontSize, &Colors::green, &Colors::white);
   mPageHeaders.add(SymbolString(symbols, AlignmentLeft));
}


void InstructionsUserInterface::renderPage2()
{
   mLoadoutInstructions.render(gScreenInfo.getGameCanvasWidth() / 2, 65, AlignmentCenter);    // Overall block is centered
}


static const char *indicatorPageHeadings[] = {
   "BADGES / ACHIEVEMENTS"
};


static void renderBadgeLine(S32 y, S32 textSize, MeritBadges badge, S32 radius, const char *name, const char *descr)
{
   S32 x = 50;

   renderBadge((F32)x, (F32)y + radius, (F32)radius, badge);
   x += radius + 10;

   glColor(Colors::yellow);
   x += drawStringAndGetWidth(x, y, textSize, name);

   glColor(Colors::cyan);
   x += drawStringAndGetWidth(x, y, textSize, " - ");

   glColor(Colors::white);
   drawString(x, y, textSize, descr);
}


struct BadgeDescr {
   MeritBadges badge;
   const char *name;
   const char *descr;
};


static S32 renderBadges(S32 y, S32 textSize, S32 descSize)
{
   // Heading
   glColor(Colors::cyan);
   drawCenteredString(y, descSize, indicatorPageHeadings[0]);
   y += 26;

   static const char *badgeHeadingDescription[] = {
      "Badges may appear next to player names on the scoreboard",
      "More will be available in future Bitfighter releases"
   };

   // Description
   glColor(Colors::green);
   drawCenteredString(y, textSize, badgeHeadingDescription[0]);
   y += 40;

   S32 radius = descSize / 2;

   static BadgeDescr badgeDescrs[] = {
      { DEVELOPER_BADGE,           "Developer",       "Have code accepted into the codebase" },
      { BADGE_TWENTY_FIVE_FLAGS,   "25 Flags",        "Return 25 flags to the Nexus" },
      { BADGE_BBB_SILVER,          "BBB Medal",       "Earn gold, silver, or bronze in a Big Bitfighter Battle" },
      { BADGE_LEVEL_DESIGN_WINNER, "Level Design",    "Win a level design contest" },
      { BADGE_ZONE_CONTROLLER,     "Zone Controller", "Capture all zones to win a Zone Control game" }
   };

   for(U32 i = 0; i < ARRAYSIZE(badgeDescrs); i++)
   {
      renderBadgeLine(y, textSize, badgeDescrs[i].badge, radius, badgeDescrs[i].name, badgeDescrs[i].descr);
      y += 26;
   }

   return y;
}


void InstructionsUserInterface::renderPageGameIndicators()
{
   S32 y = 40;
   S32 descSize = 20;
   S32 textSize = 17;

   y = renderBadges(y, textSize, descSize);
}


static const char *moduleInstructions[] = {
   "Modules have up to 3 modes: Passive, Active, and Kinetic (P/A/K)",
   "Passive mode is always active and costs no energy (e.g. Armor).",
   "Use Active mode by pressing module's activation key (e.g. Shield).",
   "Double-click activation key to use module's Kinetic mode.",
};

static const char *moduleDescriptions[][2] = {
   { "Boost: ",    "Turbo (A), Pulse (K)" },
   { "Shield: ",   "Reflects incoming projectiles (A)" },
   { "Armor: ",    "Reduces damage, makes ship harder to control (P)" },
   { "",           "Incoming bouncers do more damage" },
   { "Repair: ",   "Repair self and nearby damaged objects (A)" },
   { "Sensor: ",   "See further and reveal hidden objects (P)" },
   { "",           "Deploy spy bugs (A)" },
   { "Cloak: ",    "Make ship invisible to enemies (A)" },
   { "Engineer: ", "Collect resources to build turrets and forcefields (A)" }
};

void InstructionsUserInterface::renderModulesPage()
{
   S32 y = 40;
   S32 textsize = 20;
   S32 textGap = 6;

   glColor(Colors::white);

   for(U32 i = 0; i < ARRAYSIZE(moduleInstructions); i++)
   {
      if(i == 2)
         glColor(Colors::green);

      drawCenteredString(y, textsize, moduleInstructions[i]);
      y += textsize + textGap;
   }

   y += textsize;

   glColor(Colors::cyan);
   drawCenteredString(y, textsize, "THE MODULES");

   y += 35;


   for(U32 i = 0; i < ARRAYSIZE(moduleDescriptions); i++)
   {
      S32 x = 105;
      glColor(Colors::yellow);
      x += drawStringAndGetWidth(x, y, textsize, moduleDescriptions[i][0]);

      // If first element is blank, it is a continution of the previous description
      if(strcmp(moduleDescriptions[i][0], "") == 0)
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
            renderShip(ShipShape::Normal, &Colors::blue, 1, thrustsBoost, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false);
            {
               F32 vertices[] = {
                     -20, -17,
                     -20, -50,
                      20, -17,
                      20, -50
               };
               F32 colors[] = {
                     1, 1, 0, 1,  // Colors::yellow
                     0, 0, 0, 1,  // Colors::black
                     1, 1, 0, 1,  // Colors::yellow
                     0, 0, 0, 1,  // Colors::black
               };
               renderColorVertexArray(vertices, colors, ARRAYSIZE(vertices) / 2, GL_LINES);
            }
            break;

         case 1:     // Shield
            renderShip(ShipShape::Normal, &Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, true, false, false, false);
            break;

         case 2:     // Armor
            renderShip(ShipShape::Normal, &Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, true);
            break;

         // skip 3 for 2nd line of armor

         case 4:     // Repair
            {
               F32 health = (getGame()->getCurrentTime() & 0x7FF) * 0.0005f;

               F32 alpha = 1.0;
               renderShip(ShipShape::Normal, &Colors::blue, alpha, thrusts, health, (F32)Ship::CollisionRadius, 0, false, false, true, false);
            }
            break;

         case 5:     // Sensor
            renderShip(ShipShape::Normal, &Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, getGame()->getCurrentTime(), false, true, false, false);
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
               renderShip(ShipShape::Normal, &Colors::blue, alpha, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false);
            }
            break;

         case 8:     // Engineer
            {
               renderShip(ShipShape::Normal, &Colors::blue, 1, thrusts, 1, (F32)Ship::CollisionRadius, 0, false, false, false, false);
               renderResourceItem(mResourceItemPoints);
            }
            break;
      }
      glPopMatrix();
      y += 45;
   }
}

const char *gGameObjectInfo[] = {
   /* 00 */   "Phaser",  "The default weapon",
   /* 01 */   "Bouncer", "Bounces off walls",
   /* 02 */   "Triple",  "Fires three diverging shots",
   /* 03 */   "Burst",   "Explosive projectile",
   /* 04 */   "Seeker",  "Homing projectile",
   /* 05 */   "", "",

   /* 06 */   "Friendly Mine",    "Team's mines show trigger radius",
   /* 07 */   "Enemy Mine",       "These are much harder to see",
   /* 08 */   "Friendly Spy Bug", "Lets you surveil the area",
   /* 09 */   "Enemy Spy Bug",    "Destroy these when you find them",
   /* 10 */   "", "",
   /* 11 */   "", "",

   /* 12 */   "Repair Item",         "Repairs damage to ship",
   /* 13 */   "Energy Item",         "Restores ship's energy",
   /* 14 */   "Neutral Turret",      "Repair to take team ownership",
   /* 15 */   "Active Turret",       "Fires at enemy team",
   /* 16 */   "Neutral Emitter",     "Repair to take team ownership",
   /* 17 */   "Force Field Emitter", "Allows only one team to pass",

   /* 18 */   "Teleporter",   "Warps ship to another location",
   /* 19 */   "Flag",         "Objective item in some game types",
   /* 20 */   "Loadout Zone", "Updates ship configuration",
   /* 21 */   "Nexus",        "Bring flags here in Nexus game",
   /* 22 */   "Goal Zone",    "Put flags and Soccer balls here",
   /* 23 */   "Asteroid",     "Silent but deadly",

   /* 24 */   "Test Item",     "Bouncy ball",
   /* 25 */   "Resource Item", "Use with engineer module",
   /* 26 */   "Soccer Ball",   "Push into enemy goal in Soccer game",
   /* 27 */   "Core",          "Kill the enemy's; defend yours OR DIE!",
   /* 28 */   "GoFast",        "Makes ship go fast"
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


      switch(i)
      {
         case 0:
            renderProjectile(Point(0,0), 0, getGame()->getCurrentTime());
            break;
         case 1:
            renderProjectile(Point(0,0), 1, getGame()->getCurrentTime());
            break;
         case 2:
            renderProjectile(Point(0,0), 2, getGame()->getCurrentTime());
            break;
         case 3:
            renderGrenade(Point(0,0), 1);
            break;
         case 4:
            renderSeeker(Point(0,0), 0, 400, getGame()->getCurrentTime());
            break;
         case 5:     // Blank
            break;
         case 6:
            renderMine(Point(0,0), true, true);
            break;
         case 7:
            renderMine(Point(0,0), true, false);
            break;
         case 8:
            renderSpyBug(Point(0,0), Colors::blue, true, true);
            break;
         case 9:
            renderSpyBug(Point(0,0), Colors::blue, false, true);
            break;
         case 10:    // Blank
         case 11:
            break;
         case 12:
            renderRepairItem(Point(0,0));
            break;
         case 13:
            renderEnergyItem(Point(0,0));
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
            {
               Vector<Point> dummy;
               renderTeleporter(Point(0,0), 0, true, getGame()->getCurrentTime(), 1, 1, (F32)Teleporter::TELEPORTER_RADIUS, 1, &dummy);
            }
            break;
         case 19:
            renderFlag(&Colors::red);
            break;
         case 20:    // Loadout zone
            {              // braces needed: see C2360
               Vector<Point> o;     // outline
               o.push_back(Point(-150, -30));
               o.push_back(Point( 150, -30));
               o.push_back(Point( 150,  30));
               o.push_back(Point(-150,  30));

               Vector<Point> f;     // fill
               Triangulate::Process(o, f);

               renderLoadoutZone(&Colors::blue, &o, &f, findCentroid(o), angleOfLongestSide(o));
            }

            break;

         case 21:    // Nexus
            {
               Vector<Point> o;     // outline
               o.push_back(Point(-150, -30));
               o.push_back(Point( 150, -30));
               o.push_back(Point( 150,  30));
               o.push_back(Point(-150,  30));

               Vector<Point> f;     // fill
               Triangulate::Process(o, f);

               renderNexus(&o, &f, findCentroid(o), angleOfLongestSide(o), 
                                       getGame()->getCurrentTime() % 5000 > 2500, 0);
            }
            break;

         case 22:    // GoalZone
            {
               Vector<Point> o;     // outline
               o.push_back(Point(-150, -30));
               o.push_back(Point( 150, -30));
               o.push_back(Point( 150,  30));
               o.push_back(Point(-150,  30));

               Vector<Point> f;     // fill
               Triangulate::Process(o, f);

               renderGoalZone(Color(0.5f, 0.5f, 0.5f), &o, &f, findCentroid(o), angleOfLongestSide(o), 
                  false, 0, 0, 0, false);
            }
            break;

         case 23:    // Asteroid... using goofball factor to keep out of sync with Nexus graphic
            renderAsteroid(Point(0,-10), 
                     (S32)(getGame()->getCurrentTime() / 2891) % Asteroid::getDesignCount(), .7f);    
            break;

         case 24:    // TestItem
            renderTestItem(mTestItemPoints);
            break;

         case 25:    // ResourceItem
            renderResourceItem(mResourceItemPoints);
            break;

         case 26:    // SoccerBall
            renderSoccerBall(Point(0,0));
            break;

         case 27:    // Core
            {
               F32 health[] = { 1,1,1,1,1,1,1,1,1,1 };
               
               Point pos(0,0);
               U32 time = U32(-1 * S32(Platform::getRealMilliseconds()));

               PanelGeom panelGeom;
               CoreItem::fillPanelGeom(pos, time, panelGeom);

               glPushMatrix();
                  glTranslate(pos);
                  glScale(.55f);
                  renderCore(pos, &Colors::blue, time, &panelGeom, health, 1.0f);
               glPopMatrix();
            }
            break;

         case 28:    // SpeedZone
            {
               Vector<Point> speedZoneRenderPoints, outlinePoints;
               SpeedZone::generatePoints(Point(-SpeedZone::height / 2, 0), Point(1, 0), 1, speedZoneRenderPoints, outlinePoints);

               renderSpeedZone(speedZoneRenderPoints, getGame()->getCurrentTime());
            }
            break;

      }
      glPopMatrix();
      objStart.y += 75;
      start.y += 75;
   }
}


extern CommandInfo chatCmds[];
extern const S32 chatCmdSize;

void InstructionsUserInterface::renderPageCommands(U32 page, const char *msg)
{
   TNLAssert(page < COMMAND_CATEGORIES, "Page too high!");

   S32 ypos = 65;

   S32 cmdCol = horizMargin;                                                         // Action column
   S32 descrCol = horizMargin + S32(gScreenInfo.getGameCanvasWidth() * 0.25) + 55;   // Control column

   ypos += mPageHeaders.render(cmdCol, ypos, AlignmentLeft) - FontSize;    // Account for different positioning of SymbolStrings and drawString()

   if(strcmp(msg, ""))
   {
      glColor(Colors::palePurple);
      drawString(cmdCol, ypos, FontSize, msg);
      ypos += 28;
   }

   Color cmdColor =   Colors::cyan;
   Color descrColor = Colors::white;
   Color secColor =   Colors::yellow;

   Color argColor;
   argColor.interp(.5, Colors::cyan, Colors::black);

   const S32 headerSize = 20;
   const S32 cmdSize = 16;
   const S32 cmdGap = 8;

   glColor(secColor);
   drawString(cmdCol,   ypos, headerSize, "Command");
   drawString(descrCol, ypos, headerSize, "Description");

   ypos += cmdSize + cmdGap;

   F32 vertices[] = {
         (F32)cmdCol, (F32)ypos,
         (F32)750,    (F32)ypos
   };
   renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, GL_LINES);

   ypos += 5;     // Small gap before cmds start

   bool first = true;
   S32 section = -1;

   for(S32 i = 0; i < ChatHelper::chatCmdSize && U32(chatCmds[i].helpCategory) <= page; i++)
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
      string args = "";

      for(S32 j = 0; j < chatCmds[i].cmdArgCount; j++)
         args += " " + chatCmds[i].helpArgString[j];

      S32 w = drawStringAndGetWidth(cmdCol, ypos, cmdSize, cmdString.c_str());
      glColor(argColor);
      drawString(cmdCol + w, ypos, cmdSize, args.c_str());

      if(chatCmds[i].lines == 1)    // Everything on one line, the normal case
      {
         glColor(descrColor);
         drawString(descrCol, ypos, cmdSize, chatCmds[i].helpTextString.c_str());
      }
      else                          // Draw the command on one line, explanation on the next, with a bit of indent
      {
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

   if(mCurPage > InstructionMaxPages - 1)
      mCurPage = 0;
}


void InstructionsUserInterface::prevPage()
{
   mCurPage--;

   if(mCurPage < 0)
      mCurPage = InstructionMaxPages - 1;
}


void InstructionsUserInterface::exitInstructions()
{
   playBoop();
   getUIManager()->reactivatePrevUI();      //mGameUserInterface
}


bool InstructionsUserInterface::onKeyDown(InputCode inputCode)
{
   if(Parent::onKeyDown(inputCode)) { /* Do nothing */ }

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
   // F1 has dual use... advance page, then quit out of help when done
   else if(checkInputCode(InputCodeManager::BINDING_HELP, inputCode))
   {
      if(mCurPage != InstructionMaxPages - 1)
         nextPage();
      else
         exitInstructions();
   }
   else if(inputCode == KEY_ESCAPE  || inputCode == BUTTON_BACK)
      exitInstructions();
   else
      return false;

   // A key was handled
   return true;
}

};
