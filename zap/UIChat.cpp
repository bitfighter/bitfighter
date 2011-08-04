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

#include "UIChat.h"
#include "UIQueryServers.h"      // For menuID
#include "input.h"
#include "masterConnection.h"
#include "UINameEntry.h"
#include "UIMenus.h"
#include "UIGame.h"  // For putting private messages into game console
#include "UIDiagnostics.h"
#include "Colors.h"
#include "ScreenInfo.h"

#include "SDL/SDL_opengl.h"

#include <stdarg.h>
#include <utility>

namespace Zap
{

Vector<StringTableEntry> AbstractChat::mPlayersInGlobalChat;

const char *ARROW = ">";
const S32 AFTER_ARROW_SPACE = 5;

extern string gPlayerName;

// Initialize some static vars
U32 AbstractChat::mColorPtr = 0;
U32 AbstractChat::mMessageCount = 0;


// By declaring these here, we avoid link errors
ChatMessage AbstractChat::mMessages[MESSAGES_TO_RETAIN];
std::map<string, Color> AbstractChat::mFromColors;       // Map nicknames to colors


AbstractChat::AbstractChat()
{
   mLineEditor = LineEditor(200);
}


Color AbstractChat::getColor(string name)
{
   if(mFromColors.count(name) == 0)    
      mFromColors[name] = getNextColor();          

   return mFromColors[name]; 
}


extern ClientInfo gClientInfo;

// We received a new incoming chat message...  Add it to the list
void AbstractChat::newMessage(string from, string message, bool isPrivate, bool isSystem)
{
   bool isFromUs = (from == gClientInfo.name);  // Is this message from us?

   // Choose a color
   Color color;

   if(isFromUs)
      color = Colors::white;                               // If so, use white
   else                                                   // Otherwise...
   {
      if(mFromColors.count(from) == 0)                   // ...see if we have a color for this nick.  If we don't, count will be 0
         mFromColors[from] = getNextColor();              // If not, get a new one

      color = mFromColors[from];
   }

   mMessages[mMessageCount % MESSAGES_TO_RETAIN] = ChatMessage(from, message, color, isPrivate, isSystem);
   mMessageCount++;

   if(isFromUs && isPrivate)
      deliverPrivateMessage(from.c_str(), message.c_str());
}


extern ClientInfo gClientInfo;

void AbstractChat::setPlayersInGlobalChat(const Vector<StringTableEntry> &playerNicks)
{
   mPlayersInGlobalChat.clear();

   for(S32 i = 0; i < playerNicks.size(); i++)
      mPlayersInGlobalChat.push_back(playerNicks[i]);
}


void AbstractChat::playerJoinedGlobalChat(const StringTableEntry &playerNick)
{
   mPlayersInGlobalChat.push_back(playerNick);

   // Make the following be from us, so it will be colored white
   newMessage(gClientInfo.name, "----- Player " + string(playerNick.getString()) + " joined the conversation -----", false, true);
   SoundSystem::playSoundEffect(SFXPlayerJoined, gIniSettings.sfxVolLevel);   // Make sound?
}


void AbstractChat::playerLeftGlobalChat(const StringTableEntry &playerNick)
{
   ChatUserInterface *ui = gClientGame->getUIManager()->getChatUserInterface();

   for(S32 i = 0; i < ui->mPlayersInGlobalChat.size(); i++)
      if(ui->mPlayersInGlobalChat[i] == playerNick)
      {
         ui->mPlayersInGlobalChat.erase_fast(i);
         newMessage(gClientInfo.name, "----- Player " + string(playerNick.getString()) + " left the conversation -----", false, true);
         SoundSystem::playSoundEffect(SFXPlayerLeft, gIniSettings.sfxVolLevel);   // Make sound?
         break;
      }
}


// We're using a rolling "wrap-around" array, and this figures out which array index we need to retrieve a message.
// First message has index == 0, second has index == 1, etc.
ChatMessage AbstractChat::getMessage(U32 index)
{
   //U32 first = (mMessageCount < MESSAGES_TO_RETAIN) ? 0 : mMessageCount % MESSAGES_TO_RETAIN;
   return mMessages[index % MESSAGES_TO_RETAIN];
}


// Retrieve the next available chat text color
Color AbstractChat::getNextColor()
{
   static const Color colorList[] = {
      Color(0.55,0.55,0),     Color(1,0.55,0.55),
      Color(0,0.6,0),         Color(0.68,1,0.25),
      Color(0,0.63,0.63),     Color(0.275,0.51,0.71),
      Color(1,1,0),           Color(0.5,0.81,0.37),
      Color(0,0.75,1),        Color(0.93,0.91,0.67),
      Color(1,0.5,1),         Color(1,0.73,0.53),
      Color(0.86,0.078,1),    Color(0.78,0.08,0.52),
      Color(0.93,0.5,0),      Color(0.63,0.32,0.18),
      Color(0.5,1,1),         Color(1,0.73,1),
      Color(0.48,0.41,0.93)
   };

   mColorPtr++;
   if(mColorPtr >= ARRAYSIZE(colorList))     // Wrap-around
      mColorPtr = 0;

   return colorList[mColorPtr];
}


// Announce we're ducking out for a spell...
void AbstractChat::leaveGlobalChat()
{
   MasterServerConnection *conn = gClientGame->getConnectionToMaster();
   if(conn)
      conn->c2mLeaveGlobalChat();
}

void AbstractChat::renderMessages(U32 ypos, U32 lineCountToDisplay)  // ypos is starting location of first message
{
   // If no messages, don't waste resources on rendering
   if (mMessageCount == 0)
      return;

   U32 firstMsg = (mMessageCount <= lineCountToDisplay) ? 0 : (mMessageCount - lineCountToDisplay);       // Don't use min/max because of U32/S32 issues!
   U32 ypos_top = ypos;
   ypos += (CHAT_FONT_SIZE + CHAT_FONT_MARGIN) * lineCountToDisplay;

   // Double pass.  First loop is just to calculate number of lines used, then second pass will render.
   bool renderLoop = false;
   do{
      for(U32 i = lineCountToDisplay - 1; i != U32_MAX; i--)
      {
         // No more rendering - we've rendered to the line count limit
         if(ypos <= ypos_top)
            break;

         // No more messages to display
         if(i >= min(firstMsg + lineCountToDisplay, mMessageCount))
            continue;  // Don't return / break, For loop is running in backwards.

         else
         {
            ChatMessage msg = getMessage(i + firstMsg);
            glColor(msg.color);

            // Figure out the x position based on the message prefixes
            S32 xpos = UserInterface::horizMargin / 2;

            xpos += UserInterface::getStringWidthf(CHAT_TIME_FONT_SIZE, "[%s] ", msg.time.c_str());
            if(!msg.isSystem)
               xpos += UserInterface::getStringWidth(CHAT_FONT_SIZE, msg.from.c_str());     // No sender for system message
            if(msg.isPrivate)
               xpos += UserInterface::getStringWidth(CHAT_FONT_SIZE, "*");
            if(!msg.isSystem)
               xpos += UserInterface::getStringWidth(CHAT_FONT_SIZE, ARROW) + AFTER_ARROW_SPACE;

            S32 allowedWidth = gScreenInfo.getGameCanvasWidth() - (2 * UserInterface::horizMargin) - xpos;

            // Calculate (and draw if in renderLoop) the message lines
            U32 lineCount = UserInterface::drawWrapText((char*)msg.message.c_str(), xpos, ypos, allowedWidth, ypos_top,
               AbstractChat::CHAT_FONT_SIZE + AbstractChat::CHAT_FONT_MARGIN,  // line height
               AbstractChat::CHAT_FONT_SIZE, // font size
               0, // how much to indent if the message is multi-line - not needed here because we already indent message
               true, // alignBottom, new messages appear below, at the bottom
               renderLoop);

            ypos -= (CHAT_FONT_SIZE + CHAT_FONT_MARGIN) * lineCount;

            // Draw the message prefixes
            if(renderLoop)
            {
               xpos = UserInterface::horizMargin / 2;
               xpos += UserInterface::drawStringAndGetWidthf((F32)xpos, F32(ypos + (CHAT_FONT_SIZE - CHAT_TIME_FONT_SIZE)) / 2.f + 2,  // + 2 just looks better!
                     CHAT_TIME_FONT_SIZE, "[%s] ", msg.time.c_str());

               if(!msg.isSystem)
                  xpos += UserInterface::drawStringAndGetWidth(xpos, ypos, CHAT_FONT_SIZE, msg.from.c_str());     // No sender for system message

               if(msg.isPrivate)
                  xpos += UserInterface::drawStringAndGetWidth(xpos, ypos, CHAT_FONT_SIZE, "*");

               if(!msg.isSystem)
                  xpos += UserInterface::drawStringAndGetWidth(xpos, ypos, CHAT_FONT_SIZE, ARROW) + AFTER_ARROW_SPACE;
            }
         }
      }

      // Calculate position for renderLoop
      ypos = ypos_top + ypos_top - ypos + (CHAT_FONT_SIZE + CHAT_FONT_MARGIN) * lineCountToDisplay;

      renderLoop = !renderLoop;
   } while(renderLoop);
}


// Render outgoing chat message composition line
void AbstractChat::renderMessageComposition(S32 ypos)
{
   const char *PROMPT_STR = "> ";     // For composition only
   const S32 promptWidth = UserInterface::getStringWidth(CHAT_FONT_SIZE, PROMPT_STR);
   const S32 xStartPos = UserInterface::horizMargin + promptWidth;

   string displayString = mLineEditor.getString();
   S32 width = UserInterface::getStringWidth(CHAT_FONT_SIZE, displayString.c_str());

   // If the string goes too far out of bounds, display it chopped off at the front to give room to type more
   while (width > gScreenInfo.getGameCanvasWidth() - (2 * UserInterface::horizMargin) - promptWidth)
   {
      displayString = displayString.substr(MESSAGE_OVERFLOW_SHIFT, string::npos);
      width = UserInterface::getStringWidth(CHAT_FONT_SIZE, displayString.c_str());
   }

   glColor3f(0,1,1);
   UserInterface::drawString(UserInterface::horizMargin, ypos, CHAT_FONT_SIZE, PROMPT_STR);

   glColor3f(1,1,1);
   UserInterface::drawString(xStartPos, ypos, CHAT_FONT_SIZE, displayString.c_str());

   mLineEditor.drawCursor(xStartPos, ypos, CHAT_FONT_SIZE, width);
}


void AbstractChat::deliverPrivateMessage(const char *sender, const char *message)
{
   // If player not in UIChat or UIQueryServers, then display message in-game if possible.  2 line message.
   UIID currId = UserInterface::current->getMenuID();
   if(currId != ChatUI && currId != QueryServersScreenUI )
   {
      gClientGame->getUserInterface()->displayChatMessage(GameUserInterface::privateF5MessageDisplayedInGameColor,
         "Private message from %s: Press [%s] to enter chat mode", sender, keyCodeToString(keyOUTGAMECHAT));
      gClientGame->getUserInterface()->displayChatMessage(GameUserInterface::privateF5MessageDisplayedInGameColor, "%s %s", ARROW, message);
   }
}


// Send chat message
void AbstractChat::issueChat()
{
   if(mLineEditor.length() > 0)
   {
      // Send message
      MasterServerConnection *conn = gClientGame->getConnectionToMaster();
      if(conn)
         conn->c2mSendChat(mLineEditor.c_str());

      // And display it locally
      newMessage(gClientInfo.name, mLineEditor.getString(), false, false);
   }
   clearChat();     // Clear message

   UserInterface::playBoop();
}


// Clear current message
void AbstractChat::clearChat()
{
   mLineEditor.clear();
}


void AbstractChat::renderChatters(S32 xpos, S32 ypos)
{
   if(mPlayersInGlobalChat.size() == 0)
      UserInterface::drawString(xpos, ypos, CHAT_NAMELIST_SIZE, "No other players currently in lobby/chat room");
   else
      for(S32 i = 0; i < mPlayersInGlobalChat.size(); i++)
      {
         const char *name = mPlayersInGlobalChat[i].getString();

         glColor(getColor(name));      // use it

         xpos += UserInterface::drawStringAndGetWidthf((F32)xpos, (F32)ypos, CHAT_NAMELIST_SIZE, "%s%s", name, (i < mPlayersInGlobalChat.size() - 1) ? "; " : "");
      }
}

////////////////////////////////////////
////////////////////////////////////////

// Constructor
ChatUserInterface::ChatUserInterface(Game *game) : Parent(game)
{
   setMenuID(GlobalChatUI);
}


void ChatUserInterface::idle(U32 timeDelta)
{
   LineEditor::updateCursorBlink(timeDelta);
}


static const S32 VERT_FOOTER_SIZE = 20;
static const S32 MENU_TITLE_SIZE = 30;
static const S32 TITLE_SUBTITLE_GAP = 5;
static const S32 MENU_SUBTITLE_SIZE = 18;

void ChatUserInterface::render()
{
   if(mRenderUnderlyingUI && getUIManager()->hasPrevUI())           // If there is an underlying menu...
   {
      getUIManager()->renderPrevUI();  // ...render it...
      glColor4f(0, 0, 0, 0.75);        // ... and dim it out a bit, nay, a lot
      glEnableBlend;
      glBegin(GL_POLYGON);
         glVertex2i(0, 0);
         glVertex2i(gScreenInfo.getGameCanvasWidth(), 0);
         glVertex2i(gScreenInfo.getGameCanvasWidth(), gScreenInfo.getGameCanvasHeight());
         glVertex2i(0, gScreenInfo.getGameCanvasHeight());
      glEnd();
      glDisableBlend;
   }

   // Render header
   renderHeader();

   // And footer
   glColor3f(0, 1, 0);
   S32 vertFooterPos = gScreenInfo.getGameCanvasHeight() - vertMargin - VERT_FOOTER_SIZE;
   drawCenteredString(vertFooterPos, VERT_FOOTER_SIZE - 2, "Type your message | ENTER to send | ESC exits");

   renderChatters(horizMargin, vertFooterPos - CHAT_NAMELIST_SIZE - CHAT_FONT_MARGIN * 2);

   // Render incoming chat msgs
   glColor3f(1,1,1);

   U32 y = UserInterface::vertMargin + 60;

   static const S32 chatAreaHeight = gScreenInfo.getGameCanvasHeight() - 2 * vertMargin -   // Screen area less margins
                     VERT_FOOTER_SIZE -                                                     // Instructions at the bottom
                     CHAT_NAMELIST_SIZE - CHAT_FONT_MARGIN * 2  -                           // Names of those in chatroom
                     MENU_TITLE_SIZE - TITLE_SUBTITLE_GAP - MENU_SUBTITLE_SIZE -            // Title/subtitle display
                     CHAT_FONT_SIZE - CHAT_FONT_MARGIN -                                    // Chat composition
                     CHAT_FONT_SIZE;                                                        // Not sure... just need a little more space??

   static const S32 MessageDisplayCount = chatAreaHeight / (CHAT_FONT_SIZE + CHAT_FONT_MARGIN);

   renderMessages(y, MessageDisplayCount);
   renderMessageComposition(vertFooterPos - 45);

   // Give user notice that there is no connection to master, and thus chatting is ineffectual
   if(!(gClientGame && gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->getConnectionState() == NetConnection::Connected))
   {
      glColor3f(1, 0, 0);
      drawCenteredString(200, 20, "Not connected to Master Server");
      drawCenteredString(230, 20, "Your chat messages cannot be relayed");
   }

   glPopMatrix();
}


void ChatUserInterface::renderHeader()
{
   // Draw title, subtitle, and footer
   glColor3f(1, 1, 1);
   drawCenteredString(vertMargin, MENU_TITLE_SIZE, "GameLobby / Global Chat");

   string subtitle = "Not currently connected to any game server";

   if(gClientGame && gClientGame->getConnectionToServer())
   {
      string name = gClientGame->getConnectionToServer()->getServerName();
      if(name == "")
         subtitle = "Connected to game server with no name";
      else
         subtitle = "Connected to game server \"" + name + "\"";
   }

   glColor3f(0, 1, 0);
   drawCenteredString(vertMargin + MENU_TITLE_SIZE + TITLE_SUBTITLE_GAP, MENU_SUBTITLE_SIZE, subtitle.c_str());
}


void ChatUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if(keyCode == keyOUTGAMECHAT)
      onOutGameChat();
   else if(keyCode == keyDIAG)            // Turn on diagnostic overlay
      getUIManager()->getDiagnosticUserInterface()->activate();
   else if(keyCode == KEY_ESCAPE)
      onEscape();
   else if (keyCode == KEY_ENTER)                // Submits message
      issueChat();
   else if (keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)       // Do backspacey things
      mLineEditor.handleBackspace(keyCode);
   else if(ascii)                               // Other keys - add key to message
      mLineEditor.addChar(ascii);
}


extern bool gDisableShipKeyboardInput;

// Run when UIChat is called in normal UI mode
void ChatUserInterface::onActivate()
{
   MasterServerConnection *conn = getGame()->getConnectionToMaster();
   if(conn)
      conn->c2mJoinGlobalChat();

   mPlayersInGlobalChat.clear();
   mRenderUnderlyingUI = true;
   gDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
}


void ChatUserInterface::onOutGameChat()
{
   onEscape();
}


void ChatUserInterface::onEscape()
{
   leaveGlobalChat();
   getUIManager()->reactivatePrevUI();
   playBoop();
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
SuspendedUserInterface::SuspendedUserInterface(Game *game) : Parent(game)
{
   setMenuID(SuspendedUI);
}


void SuspendedUserInterface::renderHeader()
{
   if(gClientGame->isSuspended())
   {
      glColor3f(1,1,1);
      drawCenteredString(vertMargin, MENU_TITLE_SIZE, "-- GAME SUSPENDED -- ");
   }
   else
   {
      glColor3f(1,0,0);
      drawCenteredString(vertMargin, MENU_TITLE_SIZE, "!! GAME RESTARTED !! ");
   }

   string subtitle = "Not currently connected to any game server";

   if(gClientGame && gClientGame->getConnectionToServer())
   {
      string name = gClientGame->getConnectionToServer()->getServerName();
      if(name == "")
         subtitle = "Connected to game server with no name";
      else
         subtitle = "Connected to game server \"" + name + "\"";
   }

   glColor3f(0, 1, 0);
   drawCenteredString(vertMargin + MENU_TITLE_SIZE + TITLE_SUBTITLE_GAP, MENU_SUBTITLE_SIZE, subtitle.c_str());
}


void SuspendedUserInterface::onOutGameChat()
{
   // Do nothing
}

};


