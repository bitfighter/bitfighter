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

#include "ChatHelper.h"

#include "ChatCommands.h"
#include "ClientGame.h"
#include "Console.h"
#include "UIChat.h"           // For font sizes and such
#include "UIInstructions.h"   // For code to activate help screen

#include "ScissorsManager.h"
#include "ScreenInfo.h"

#include "Colors.h"

#include "RenderUtils.h"
#include "OpenglUtils.h"
#include "stringUtils.h"

namespace Zap
{
   CommandInfo chatCmds[] = {   
   //  cmdName          cmdCallback                 cmdArgInfo cmdArgCount   helpCategory helpGroup lines,  helpArgString            helpTextString

   { "dlmap",    &ChatCommands::downloadMapHandler, { STR },       1,      ADV_COMMANDS,     0,     1,    {"<level>"},            "Download the level from the online level database" },
   { "password", &ChatCommands::submitPassHandler,  { STR },       1,      ADV_COMMANDS,     0,     1,    {"<password>"},         "Request admin or level change permissions"  },
   { "servvol",  &ChatCommands::servVolHandler,     { xINT },      1,      ADV_COMMANDS,     0,     1,    {"<0-10>"},             "Set volume of server"  },
   { "getmap",   &ChatCommands::getMapHandler,      { STR },       1,      ADV_COMMANDS,     1,     1,    {"[file]"},             "Save currently playing level in [file], if allowed" },
   { "idle",     &ChatCommands::idleHandler,        {  },          0,      ADV_COMMANDS,     1,     1,    {  },                   "Place client in idle mode (AFK)" },
   { "pm",       &ChatCommands::pmHandler,          { NAME, STR }, 2,      ADV_COMMANDS,     1,     1,    {"<name>","<message>"}, "Send private message to player" },
   { "mute",     &ChatCommands::muteHandler,        { NAME },      1,      ADV_COMMANDS,     1,     1,    {"<name>"},             "Toggle hiding chat messages from <name>" },
   { "vmute",    &ChatCommands::voiceMuteHandler,   { NAME },      1,      ADV_COMMANDS,     1,     1,    {"<name>"},             "Toggle muting voice chat from <name>" },
                 
   { "mvol",     &ChatCommands::mVolHandler,      { xINT },      1,      SOUND_COMMANDS,   2,     1,    {"<0-10>"},             "Set music volume"      },
   { "svol",     &ChatCommands::sVolHandler,      { xINT },      1,      SOUND_COMMANDS,   2,     1,    {"<0-10>"},             "Set SFX volume"        },
   { "vvol",     &ChatCommands::vVolHandler,      { xINT },      1,      SOUND_COMMANDS,   2,     1,    {"<0-10>"},             "Set voice chat volume" },
   { "mnext",    &ChatCommands::mNextHandler,     {  },          0,      SOUND_COMMANDS,   2,     1,    {  },                   "Play next track in the music list" },
   { "mprev",    &ChatCommands::mPrevHandler,     {  },          0,      SOUND_COMMANDS,   2,     1,    {  },                   "Play previous track in the music list" },

   { "add",         &ChatCommands::addTimeHandler,         { xINT },                 1, LEVEL_COMMANDS,  0,  1,  {"<time in minutes>"},                      "Add time to the current game" },
   { "next",        &ChatCommands::nextLevelHandler,       {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Start next level" },
   { "prev",        &ChatCommands::prevLevelHandler,       {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Replay previous level" },
   { "restart",     &ChatCommands::restartLevelHandler,    {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Restart current level" },
   { "random",      &ChatCommands::randomLevelHandler,     {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Start random level" },
   { "settime",     &ChatCommands::setTimeHandler,         { xINT },                 1, LEVEL_COMMANDS,  0,  1,  {"<time in minutes>"},                      "Set play time for the level" },
   { "setscore",    &ChatCommands::setWinningScoreHandler, { xINT },                 1, LEVEL_COMMANDS,  0,  1,  {"<score>"},                                "Set score to win the level" },
   { "resetscore",  &ChatCommands::resetScoreHandler,      {  },                     0, LEVEL_COMMANDS,  0,  1,  {  },                                       "Reset all scores to zero" },
   { "addbot",      &ChatCommands::addBotHandler,          { STR, TEAM, STR },       3, LEVEL_COMMANDS,  1,  2,  {"[file]", "[team name or num]","[args]"},          "Add bot from [file] to [team num], pass [args] to bot" },
   { "addbots",     &ChatCommands::addBotsHandler,         { xINT, STR, TEAM, STR }, 4, LEVEL_COMMANDS,  1,  2,  {"[count]","[file]","[team name or num]","[args]"}, "Add [count] bots from [file] to [team num], pass [args] to bot" },
   { "kickbot",     &ChatCommands::kickBotHandler,         {  },                     0, LEVEL_COMMANDS,  1,  1,  {  },                                       "Kick most recently added bot" },
   { "kickbots",    &ChatCommands::kickBotsHandler,        {  },                     0, LEVEL_COMMANDS,  1,  1,  {  },                                       "Kick all bots" },

   { "announce",           &ChatCommands::announceHandler,           { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<announcement>"},      "Announce an important message" },
   { "kick",               &ChatCommands::kickPlayerHandler,         { NAME },       1, ADMIN_COMMANDS,  0,  1,  {"<name>"},              "Kick a player from the game" },
   { "ban",                &ChatCommands::banPlayerHandler,          { NAME, xINT }, 2, ADMIN_COMMANDS,  0,  1,  {"<name>","[duration]"}, "Ban a player from the server (IP-based, def. = 60 mins)" },
   { "banip",              &ChatCommands::banIpHandler,              { STR, xINT },  2, ADMIN_COMMANDS,  0,  1,  {"<ip>","[duration]"},   "Ban an IP address from the server (def. = 60 mins)" },
   { "setlevpass",         &ChatCommands::setLevPassHandler,         { STR },        1, ADMIN_COMMANDS,  0,  1,  {"[passwd]"},            "Set level change password (use blank to clear)" },
   { "setserverpass",      &ChatCommands::setServerPassHandler,      { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<passwd>"},            "Set server password (use blank to clear)" },
   { "leveldir",           &ChatCommands::setLevelDirHandler,        { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<new level folder>"},  "Set leveldir param on the server (changes levels available)" },
   { "setservername",      &ChatCommands::setServerNameHandler,      { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<name>"},              "Set server name" },
   { "setserverdescr",     &ChatCommands::setServerDescrHandler,     { STR },        1, ADMIN_COMMANDS,  0,  1,  {"<descr>"},             "Set server description" },
   { "deletecurrentlevel", &ChatCommands::deleteCurrentLevelHandler, { },            0, ADMIN_COMMANDS,  0,  1,  {""},                    "Remove current level from server" },
   { "gmute",              &ChatCommands::globalMuteHandler,         { NAME },       1, ADMIN_COMMANDS,  0,  1,  {"<name>"},              "Globally mute/unmute a player" },
   { "rename",             &ChatCommands::renamePlayerHandler,       { NAME, STR },  2, ADMIN_COMMANDS,  0,  1,  {"<from>","<to>"},       "Give a player a new name" },
   { "maxbots",            &ChatCommands::setMaxBotsHandler,         { xINT },       1, ADMIN_COMMANDS,  0,  1,  {"<count>"},             "Set the maximum bots allowed for this server" },
   { "shuffle",            &ChatCommands::shuffleTeams,              { },            0, ADMIN_COMMANDS,  0,  1,   { "" },                 "Randomly reshuffle teams" },

   { "setownerpass",       &ChatCommands::setOwnerPassHandler,       { STR },        1, OWNER_COMMANDS,  0,  1,  {"[passwd]"},            "Set owner password" },
   { "setadminpass",       &ChatCommands::setAdminPassHandler,       { STR },        1, OWNER_COMMANDS,  0,  1,  {"[passwd]"},            "Set admin password" },
   { "shutdown",           &ChatCommands::shutdownServerHandler,     { xINT, STR },  2, OWNER_COMMANDS,  0,  1,  {"[time]","[message]"},  "Start orderly shutdown of server (def. = 10 secs)" },

   { "showcoords", &ChatCommands::showCoordsHandler,    {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show ship coordinates" },
   { "showzones",  &ChatCommands::showZonesHandler,     {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show bot nav mesh zones" },
   { "showids",    &ChatCommands::showIdsHandler,       {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show object ids" },
   { "showpaths",  &ChatCommands::showPathsHandler,     {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show robot paths" },
   { "showbots",   &ChatCommands::showBotsHandler,      {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Show all robots" },
   { "pausebots",  &ChatCommands::pauseBotsHandler,     {  },        0, DEBUG_COMMANDS, 0,  1, {  },         "Pause all bots. Reissue to start again" },
   { "stepbots",   &ChatCommands::stepBotsHandler,      { xINT },    1, DEBUG_COMMANDS, 1,  1, {"[steps]"},  "Advance bots by number of steps (default = 1)"},
   { "linewidth",  &ChatCommands::lineWidthHandler,     { xINT },    1, DEBUG_COMMANDS, 1,  1, {"[number]"}, "Change width of all lines (default = 2)" },
   { "maxfps",     &ChatCommands::maxFpsHandler,        { xINT },    1, DEBUG_COMMANDS, 1,  1, {"<number>"}, "Set maximum speed of game in frames per second" },
   { "lag",        &ChatCommands::lagHandler, {xINT,xINT,xINT,xINT}, 4, DEBUG_COMMANDS, 1,  2, {"<send lag>", "[% of send drop packets]", "[receive lag]", "[% of receive drop packets]"}, "Set additional lag and percent of dropped packets" },
   { "clearcache", &ChatCommands::clearCacheHandler,    {  },        0, DEBUG_COMMANDS, 1,  1, { },          "Clear any cached scripts, forcing them to be reloaded" },
};


const S32 ChatHelper::chatCmdSize = ARRAYSIZE(chatCmds); // So instructions will now how big chatCmds is


static void makeCommandCandidateList();      // Forward delcaration

ChatHelper::ChatHelper() : mLineEditor(200)
{
   mCurrentChatType = NoChat;
   makeCommandCandidateList();

   setAnimationTime(65);    // Menu appearance time
}

// Destructor
ChatHelper::~ChatHelper()
{
   // Do nothing
}


HelperMenu::HelperMenuType ChatHelper::getType() { return ChatHelperType; }


void ChatHelper::activate(ChatType chatType)
{
   mCurrentChatType = chatType;
   getGame()->setBusyChatting(true);
}


extern ScreenInfo gScreenInfo;

bool ChatHelper::isCmdChat()
{
   return mLineEditor.at(0) == '/' || mCurrentChatType == CmdChat;
}


void ChatHelper::render()
{
   const char *promptStr;

   Color baseColor;

   if(isCmdChat())      // Whatever the underlying chat mode, seems we're entering a command here
   {
      baseColor = Colors::cmdChatColor;
      promptStr = mCurrentChatType ? "(Command): /" : "(Command): ";
   }
   else if(mCurrentChatType == TeamChat)    // Team chat (goes to all players on team)
   {
      baseColor = Colors::teamChatColor;
      promptStr = "(Team): ";
   }
   else                                     // Global in-game chat (goes to all players in game)
   {
      baseColor = Colors::globalChatColor;
      promptStr = "(Global): ";
   }

   // Protect against crashes while game is initializing... is this really needed??
   if(!getGame()->getConnectionToServer())
      return;

   // Size of chat composition elements
   static const S32 CHAT_COMPOSE_FONT_SIZE = 12;
//   static const S32 CHAT_COMPOSE_FONT_GAP = CHAT_COMPOSE_FONT_SIZE / 4;

   static const S32 BOX_HEIGHT = CHAT_COMPOSE_FONT_SIZE + 10;

   S32 xPos = UserInterface::horizMargin;

   // Define some vars for readability:
   S32 promptSize = getStringWidth(CHAT_COMPOSE_FONT_SIZE, promptStr);
   S32 nameSize   = getStringWidthf(CHAT_COMPOSE_FONT_SIZE, "%s: ", getGame()->getClientInfo()->getName().getString());
   S32 nameWidth  = max(nameSize, promptSize);
   // Above block repeated below...


   S32 ypos = IN_GAME_CHAT_DISPLAY_POS + CHAT_COMPOSE_FONT_SIZE + 11;      // Top of the box when fully displayed
   S32 realYPos = ypos;

   bool isAnimating = isOpening() || isClosing();

   // Adjust for animated effect
   if(isAnimating)
      ypos += S32((getFraction()) * BOX_HEIGHT);

   S32 boxWidth = gScreenInfo.getGameCanvasWidth() - 2 * UserInterface::horizMargin - (nameWidth - promptSize) - 230;

   // Reuse this to avoid startup and breakdown costs
   static ScissorsManager scissorsManager;

   // Only need to set scissors if we're scrolling.  When not scrolling, we control the display by only showing
   // the specified number of lines; there are normally no partial lines that need vertical clipping as 
   // there are when we're scrolling.  Note also that we only clip vertically, and can ignore the horizontal.
   scissorsManager.enable(isAnimating, getGame()->getSettings()->getIniSettings()->displayMode, 
                          0, realYPos - 3, gScreenInfo.getGameCanvasWidth(), BOX_HEIGHT);

   // Render text entry box like thingy
   TNLAssert(glIsEnabled(GL_BLEND), "Why is blending off here?");

   F32 top = (F32)ypos - 3;

   F32 vertices[] = {
         (F32)xPos,            top,
         (F32)xPos + boxWidth, top,
         (F32)xPos + boxWidth, top + BOX_HEIGHT,
         (F32)xPos,            top + BOX_HEIGHT
   };

   for(S32 i = 1; i >= 0; i--)
   {
      glColor(baseColor, i ? .25f : .4f);
      renderVertexArray(vertices, ARRAYSIZE(vertices) / 2, i ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
   }

   glColor(baseColor);

   // Display prompt
   S32 promptWidth = getStringWidth(CHAT_COMPOSE_FONT_SIZE, promptStr);
   S32 xStartPos   = xPos + 3 + promptWidth;

   drawString(xPos + 3, ypos, CHAT_COMPOSE_FONT_SIZE, promptStr);  // draw prompt

   // Display typed text
   string displayString = mLineEditor.getString();
   S32 displayWidth = getStringWidth(CHAT_COMPOSE_FONT_SIZE, displayString.c_str());

   // If the string goes too far out of bounds, display it chopped off at the front to give more room to type
   while (displayWidth > boxWidth - promptWidth - 16)  // 16 -> Account for margin and cursor
   {
      displayString = displayString.substr(25, string::npos);  // 25 -> # chars to chop off displayed text if overflow
      displayWidth = getStringWidth(CHAT_COMPOSE_FONT_SIZE, displayString.c_str());
   }

   drawString(xStartPos, ypos, CHAT_COMPOSE_FONT_SIZE, displayString.c_str());

   // If we've just finished entering a chat cmd, show next parameter
   if(isCmdChat())
   {
      string line = mLineEditor.getString();
      Vector<string> words = parseStringAndStripLeadingSlash(line.c_str());

      if(words.size() > 0)
      {
         for(S32 i = 0; i < chatCmdSize; i++)
         {
            const char *cmd = words[0].c_str();

            if(!stricmp(cmd, chatCmds[i].cmdName.c_str()))
            {
               // My thinking here is that if the number of quotes is odd, the last argument is not complete, even if
               // it ends in a space.  There may be an edge case that voids this argument, but our use is simple enough 
               // that this should work well.  If a number is even, num % 2 will be 0.
               S32 numberOfQuotes = count(line.begin(), line.end(), '"');
               if(chatCmds[i].cmdArgCount >= words.size() && line[line.size() - 1] == ' ' && numberOfQuotes % 2 == 0)
               {
                  glColor(baseColor * .5);
                  drawString(xStartPos + displayWidth, ypos, CHAT_COMPOSE_FONT_SIZE, chatCmds[i].helpArgString[words.size() - 1].c_str());
               }

               break;
            }
         }
      }
   }

   glColor(baseColor);
   mLineEditor.drawCursor(xStartPos, ypos, CHAT_COMPOSE_FONT_SIZE, displayWidth);

   // Restore scissors settings -- only used during scrolling
   scissorsManager.disable();
}


void ChatHelper::onActivated()
{
   Parent::onActivated();
}


// When chatting, show command help if user presses F1
void ChatHelper::activateHelp(UIManager *uiManager)
{
    getGame()->getUIManager()->getInstructionsUserInterface()->activatePage(InstructionsUserInterface::InstructionAdvancedCommands);
}


// Make a list of all players in the game
static void makePlayerNameList(Game *game, Vector<string> &nameCandidateList)
{
   nameCandidateList.clear();

   for(S32 i = 0; i < game->getClientCount(); i++)
      nameCandidateList.push_back(((Game *)game)->getClientInfo(i)->getName().getString());
}


static Vector<string> commandCandidateList;

static void makeTeamNameList(const Game *game, Vector<string> &nameCandidateList)
{
   nameCandidateList.clear();

   for(S32 i = 0; i < game->getTeamCount(); i++)
      nameCandidateList.push_back(game->getTeamName(i).getString());
}


static Vector<string> *getCandidateList(Game *game, const char *first, S32 arg)
{
   if(arg == 0)         // ==> Command completion
      return &commandCandidateList;

   else if(arg > 0)     // ==> Arg completion
   {
      // Figure out which command we're entering, so we know what kind of args to expect
      S32 cmd = -1;

      for(S32 i = 0; i < ChatHelper::chatCmdSize; i++)
         if(!stricmp(chatCmds[i].cmdName.c_str(), first))
         {
            cmd = i;
            break;
         }

      if(cmd != -1 && arg <= chatCmds[cmd].cmdArgCount)     // Found a command
      {
         ArgTypes argType = chatCmds[cmd].cmdArgInfo[arg - 1];  // What type of arg are we expecting?

         static Vector<string> nameCandidateList;     // Reusable container

         if(argType == NAME)           // ==> Player name completion
         {  
            makePlayerNameList(game, nameCandidateList);    // Creates a list of all player names
            return &nameCandidateList;
         }

         else if(argType == TEAM)      // ==> Team name completion
         {
            makeTeamNameList(game, nameCandidateList);
            return &nameCandidateList;
         }
         // else no arg completion for you!
      }
   }
   
   return NULL;                        // ==> No completion options
}


// Returns true if key was used, false if not
bool ChatHelper::processInputCode(InputCode inputCode)
{
   // Check for backspace before processing parent because parent will use backspace to close helper, but we want to use
   // it as a, well, a backspace key!
   if(inputCode == KEY_BACKSPACE)
      mLineEditor.backspacePressed();
   else if(Parent::processInputCode(inputCode))
      return true;
   else if(inputCode == KEY_ENTER)
      issueChat();
   else if(inputCode == KEY_DELETE)
      mLineEditor.deletePressed();
   else if(inputCode == KEY_TAB)      // Auto complete any commands
   {
      if(isCmdChat())     // It's a command!  Complete!  Complete!
      {
         // First, parse line into words
         Vector<string> words = parseString(mLineEditor.getString());

         bool needLeadingSlash = false;

         // Handle leading slash when command is entered from ordinary chat prompt
         if(words.size() > 0 && words[0][0] == '/')
         {
            // Special case: User has entered process by starting with global chat, and has typed "/" then <tab>
            if(mLineEditor.getString() == "/")
               words.clear();          // Clear -- it's as if we're at a fresh "/" prompt where the user has typed nothing
            else
               words[0].erase(0, 1);   // Strip char -- remove leading "/" so it's as if were at a regular "/" prompt

            needLeadingSlash = true;   // We'll need to add the stripped "/" back in later
         }
               
         S32 arg;                 // Which word we're looking at
         string partial;          // The partially typed word we're trying to match against
         const char *first;       // First arg we entered (will match partial if we're still entering the first one)
         
         // Check for trailing space --> http://www.suodenjoki.dk/us/archive/2010/basic-string-back.htm
         if(words.size() > 0 && *mLineEditor.getString().rbegin() != ' ')   
         {
            arg = words.size() - 1;          // No trailing space --> current arg is the last word we've been typing
            partial = words[arg];            // We'll be matching against what we've typed so far
            first = words[0].c_str();      
         }
         else if(words.size() > 0)           // We've entered a word, pressed space indicating word is complete, 
         {                                   // but have not started typing the next word.  We'll let user cycle through every
            arg = words.size();              // possible value for next argument.
            partial = "";
            first = words[0].c_str(); 
         }
         else     // If the editor is empty, or if final character is a space, then we need to set these params differently
         {
            arg = words.size();              // Trailing space --> current arg is the next word we've not yet started typing
            partial = "";
            first = "";                      // We'll be matching against an empty list since we've typed nothing so far
         }
         
         const string *entry = mLineEditor.getStringPtr();  

         Vector<string> *candidates = getCandidateList(getGame(), first, arg);     // Could return NULL

         // If the command string has quotes in it, use the last space up to the first quote
         std::size_t lastChar = string::npos;
         if(entry->find_first_of("\"") != string::npos)
            lastChar = entry->find_first_of("\"");

         string appender = " ";

         std::size_t pos = entry->find_last_of(' ', lastChar);

         if(pos == string::npos)                         // String does not contain a space, requires special handling
         {
            pos = 0;
            if(words.size() <= 1 && needLeadingSlash)    // ugh!  More special cases!
               appender = "/";
            else
               appender = "";
         }

         mLineEditor.completePartial(candidates, partial, pos, appender); 
      }
   }
   else
      return false;

   return true;
}


const char *ChatHelper::getChatMessage() const
{
   return mLineEditor.c_str();
}


extern S32 QSORT_CALLBACK alphaSort(string *a, string *b);     // Sort alphanumerically

static void makeCommandCandidateList()
{
   for(S32 i = 0; i < ChatHelper::chatCmdSize; i++)
      commandCandidateList.push_back(chatCmds[i].cmdName);

   commandCandidateList.sort(alphaSort);
}


void ChatHelper::onTextInput(char ascii)
{
   // Pass the key on to the console for processing
   if(gConsole.onKeyDown(ascii))
      return;

   // Make sure we have a chat box open
   if(mCurrentChatType != NoChat)
      // Append any keys to the chat message
      if(ascii)
         // Protect against crashes while game is initializing (because we look at the ship for the player's name)
         if(getGame()->getConnectionToServer())     // getGame() cannot return NULL here
            mLineEditor.addChar(ascii);
}


// User has finished entering a chat message and pressed <enter>
void ChatHelper::issueChat()
{
   TNLAssert(mCurrentChatType != NoChat, "Not in chat mode!");

   if(!mLineEditor.isEmpty())
   {
      // Check if chat buffer holds a message or a command
      if(isCmdChat())    // It's a command
         runCommand(getGame(), mLineEditor.c_str());
      else               // It's a chat message
         getGame()->sendChat(mCurrentChatType == GlobalChat, mLineEditor.c_str());   // Broadcast message
         
   }

   exitHelper();     // Hide chat display
}


// Process a command entered at the chat prompt
// Returns true if command was handled (even if it was bogus); returning false will cause command to be passed on to the server
// Runs on client; returns true unless we don't want to undelay a delayed spawn when command is entered
// Static method
void ChatHelper::runCommand(ClientGame *game, const char *input)
{
   Vector<string> words = parseStringAndStripLeadingSlash(input); 

   if(words.size() == 0)            // Just in case, must have 1 or more words to check the first word as command
      return;

   GameConnection *gc = game->getConnectionToServer();

   if(!gc)
   {
      game->displayErrorMessage("!!! Not connected to server");
      return;
   }

   for(U32 i = 0; i < ARRAYSIZE(chatCmds); i++)
      if(lcase(words[0]) == chatCmds[i].cmdName)
      {
         (*(chatCmds[i].cmdCallback))(game, words);
         return; 
      }

   serverCommandHandler(game, words);     // Command unknown to client, will pass it on to server

   return;
}


// Use this method when you need to keep client/server compatibility between bitfighter
// versions (e.g. 015 -> 015a)
// If you are working on a new version (e.g. 016), then create an appropriate c2s handler function
// Static method
void ChatHelper::serverCommandHandler(ClientGame *game, const Vector<string> &words)
{
   Vector<StringPtr> args;

   for(S32 i = 1; i < words.size(); i++)
      args.push_back(StringPtr(words[i]));

   game->sendCommand(StringTableEntry(words[0], false), args);
}


// Need to handle the case where you do /idle while spawn delayed... you should NOT exit from spawn delay in that case
void ChatHelper::exitHelper()
{
   Parent::exitHelper();

   mLineEditor.clear();
   getGame()->setBusyChatting(false);
}


bool ChatHelper::isMovementDisabled() const { return true;  }
bool ChatHelper::isChatDisabled()     const { return false; }


};

