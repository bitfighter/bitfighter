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

#include "../glut/glutInclude.h"
#include <stdarg.h>

namespace Zap
{

const char *arrow = " -> ";

// Initialize some static vars
U32 AbstractChat::mColorPtr = 0;
U32 AbstractChat::mMessageCount = 0;

// By declaring these here, we avoid link errors
ChatMessage AbstractChat::mMessages[MessagesToRetain];
std::map<string, Color, strCmp> AbstractChat::mFromColors;       // Map nicknames to colors



// We received a new incoming chat message...  Add it to the list
void AbstractChat::newMessage(string from, string message, bool isPrivate)
{
   bool isFromUs = (from == string(gNameEntryUserInterface.getText()));  // Is this message from us?

   // Choose a color
   Color color;

   if(isFromUs)
      color = Color(1,1,1);                               // If so, use white
   else                                                   // Otherwise...
   {
      if (mFromColors.count(from) == 0)                   // ...see if we have a color for this nick.  If we don't, count will be 0
         mFromColors[from] = getNextColor();              // If not, get a new one

      color = mFromColors[from];
   }

   mMessages[mMessageCount % MessagesToRetain] = ChatMessage(from, message, color, isPrivate);
   mMessageCount++;

   if(isFromUs && isPrivate)
      deliverPrivateMessage(from.c_str(), message.c_str());
}

 
// We're using a rolling "wrap-around" array, and this figures out which array index we need to retrieve a message.
// First message has index == 0, second has index == 1, etc.
ChatMessage AbstractChat::getMessage(U32 index) 
{ 
   U32 first = (mMessageCount < MessagesToRetain) ? 0 : mMessageCount % MessagesToRetain;
   return mMessages[(first + index) % MessagesToRetain]; 
}


// Retrieve the next available chat text color
Color AbstractChat::getNextColor()
{
   static Color colorList[] = {
      Color(0.55,0.55,0),     Color(1,0.55,0.55),
      Color(0,0.6,0),         Color(0.68,1,0.25),
      Color(0,0.63,0.63),     Color(0.275,0.51,0.71),
      Color(1,1,0),           Color(0.5,0.81,0.37),
      Color(0,0.75,1),        Color(0.93,0.91,0.67),
      Color(1,0.5,1),         Color(1,0.73,0.53),
      Color(0.86,0.078,1),    Color(0.78,0.08,0.52),
      Color(0.93,0.5,0),      Color(0.63,0.32,0.18),
      Color(0.5,1,1),         Color(1,0.73,1),
      Color(0.48,0.41,0.93),  NULL
   };

   mColorPtr++;
   if(colorList[mColorPtr] == NULL)
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


void AbstractChat::renderMessage(U32 index, U32 fontsize, U32 yPos, U32 numberToDisplay)            // yPos is starting location of first message
{
   U32 firstMsg = (mMessageCount <= numberToDisplay) ? 0 : (mMessageCount - numberToDisplay);       // Don't use min/max because of U32/S32 issues!

   if(index >= min(firstMsg + numberToDisplay, mMessageCount))       // No more messages to display
      return;

   ChatMessage msg = getMessage(index + firstMsg); 
   glColor(msg.color);

   yPos += index * (fontsize + 4);   // 4 = vertical padding between messages
   UserInterface::drawString(UserInterface::horizMargin, yPos, fontsize, msg.from.c_str());

   S32 fromWidth = UserInterface::getStringWidth(fontsize, msg.from.c_str());

   if(msg.isPrivate)
   {
      UserInterface::drawString(UserInterface::horizMargin + fromWidth, yPos, fontsize, "*");
      fromWidth += UserInterface::getStringWidth(fontsize, "*");
   }

   UserInterface::drawString(UserInterface::horizMargin + fromWidth, yPos, fontsize, arrow);

   fromWidth += UserInterface::getStringWidth(fontsize, arrow);
   UserInterface::drawString(UserInterface::horizMargin + fromWidth, yPos, fontsize, msg.message.c_str());
}


void AbstractChat::deliverPrivateMessage(const char *sender, const char *message)
{
   // If player not in UIChat or UIQueryServers, then display message in-game if possible.  2 line message. 
   if(UserInterface::current->getMenuID() != gChatInterface.getMenuID() &&
      UserInterface::current->getMenuID() != gQueryServersUserInterface.getMenuID() )    
   {
      gGameUserInterface.displayMessage(GameUserInterface::privateF5MessageDisplayedInGameColor, 
         "Private message from %s: Press [%s] to enter chat mode", sender, keyCodeToString(keyOUTGAMECHAT));
      gGameUserInterface.displayMessage(GameUserInterface::privateF5MessageDisplayedInGameColor, "%s%s", arrow, message);
   }
}

///////////////////////////////////////////

// Constructor
ChatUserInterface::ChatUserInterface()
{
   setMenuID(GlobalChatUI);

   menuTitle = "GameLobby / Global Chat";
   menuFooter = "Type your message | ENTER to send | ESC exits";
}


void ChatUserInterface::idle(U32 timeDelta)
{
   updateCursorBlink(timeDelta);
}


void ChatUserInterface::render()
{
   if (prevUIs.size())           // If there is an underlying menu...
   {
      prevUIs.last()->render();  // ...render it...
      glColor4f(0, 0, 0, 0.75);  // ... and dim it out a bit, nay, a lot
      glEnable(GL_BLEND);
      glBegin(GL_POLYGON);
         glVertex2f(0, 0);
         glVertex2f(canvasWidth, 0);
         glVertex2f(canvasWidth, canvasHeight);
         glVertex2f(0, canvasHeight);
      glEnd();
      glDisable(GL_BLEND);
   }

   // Draw title, subtitle, and footer
   glColor3f(1, 1, 1);
   drawCenteredString(vertMargin, 30, menuTitle);

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
   drawCenteredString(vertMargin + 35, 18, subtitle.c_str());

   S32 vertFooterPos = canvasHeight - vertMargin - 20;
   drawCenteredString(vertFooterPos, 18, menuFooter);

   S32 xpos = horizMargin;
   S32 ypos = vertFooterPos - 16;

   glColor3f(1,1,0);

   if(mPlayersInGlobalChat.size() == 0)
      drawString(xpos, ypos, 11, "No other players currently in lobby/chat room");
   else
      for(S32 i = 0; i < mPlayersInGlobalChat.size(); i++)
      {
         drawStringf(xpos, ypos, 11, "%s%s", mPlayersInGlobalChat[i].getString(), (i < mPlayersInGlobalChat.size() - 1) ? "; " : "");
         xpos += getStringWidthf(11, "%s ;",mPlayersInGlobalChat[i].getString());
      }

   // Render incoming chat msgs
   glColor3f(1,1,1);

   U32 y = UserInterface::vertMargin + 60;

   static const U32 MessageDisplayCount = 22;

   for(U32 i = 0; i < MessageDisplayCount; i++)
      renderMessage(i, GlobalChatFontSize, y, MessageDisplayCount);

   // Render outgoing chat message composition line
   const char promptStr[] = "> ";

   S32 horizChatPos = UserInterface::horizMargin + getStringWidth(GlobalChatFontSize, promptStr);
   S32 vertChatPos = vertFooterPos - 45;

   glColor3f(0,1,1);
   drawString(UserInterface::horizMargin, vertChatPos, GlobalChatFontSize, promptStr);

   glColor3f(1,1,1);
   drawStringf(horizChatPos, vertChatPos, GlobalChatFontSize, "%s%s", mChatBuffer, cursorBlink ? "_" : " ");

   // Give user notice that there is no connection to master, and thus chatting is ineffectual
   if(!(gClientGame && gClientGame->getConnectionToMaster() && gClientGame->getConnectionToMaster()->getConnectionState() == NetConnection::Connected))
   {
      glColor3f(1, 0, 0);
      drawCenteredString(200, 20, "Not connected to Master Server");
      drawCenteredString(230, 20, "Your chat messages cannot be relayed");
   }

   glPopMatrix();
}


void ChatUserInterface::onKeyDown(KeyCode keyCode, char ascii)
{
   if (keyCode == keyOUTGAMECHAT)
   {
      UserInterface::playBoop();
      onEscape();
   }
   else if (keyCode == KEY_ENTER)                 // Submits message
   {
      UserInterface::playBoop();
      issueChat();
   }
   else if (keyCode == KEY_ESCAPE)               // Exits chat
   {
      UserInterface::playBoop();
      onEscape();
   }
   else if (keyCode == KEY_DELETE || keyCode == KEY_BACKSPACE)       // Do backspacey things
   {
      if(mChatCursorPos > 0)
      {
         mChatCursorPos--;
         for(U32 i = mChatCursorPos; mChatBuffer[i]; i++)
            mChatBuffer[i] = mChatBuffer[i+1];
      }
   }
   else if(ascii)                               // Other keys - add key to message
   {
      for(U32 i = sizeof(mChatBuffer) - 2; i > mChatCursorPos; i--)  // If inserting...
         mChatBuffer[i] = mChatBuffer[i-1];                          // ...move chars forward

      // Limit chat messages to the size that can be displayed on the screen

      S32 fromWidth = getStringWidthf(GlobalChatFontSize, "%s%s", gNameEntryUserInterface.getText(), arrow);
      if((mChatCursorPos < sizeof(mChatBuffer) - 2 )  && fromWidth + (S32) getStringWidthf(GlobalChatFontSize, "%s%c", mChatBuffer, ascii) < UserInterface::canvasWidth - 2 * horizMargin )
      {
         mChatBuffer[mChatCursorPos] = ascii;
         mChatCursorPos++;
      }
   }
}


// Send chat message
void ChatUserInterface::issueChat()
{
   if(mChatBuffer[0])
   {
      // Send message
      MasterServerConnection *conn = gClientGame->getConnectionToMaster();
      if(conn)
         conn->c2mSendChat(mChatBuffer);

      // And display it locally
      newMessage(gNameEntryUserInterface.getText(), mChatBuffer, false);
   }
   cancelChat();     // Clear message
}


// Clear current message
void ChatUserInterface::cancelChat()
{
   memset(mChatBuffer, 0, sizeof(mChatBuffer));
   mChatCursorPos = 0;
}


extern bool gDisableShipKeyboardInput;

// Run when UIChat is called in normal UI mode
void ChatUserInterface::onActivate()
{
   MasterServerConnection *conn = gClientGame->getConnectionToMaster();
   if(conn)
      conn->c2mJoinGlobalChat();

   mPlayersInGlobalChat.clear();
   gDisableShipKeyboardInput = true;       // Keep keystrokes from getting to game
}


void ChatUserInterface::onEscape()
{
   leaveGlobalChat();
   UserInterface::reactivatePrevUI();
}

ChatUserInterface gChatInterface;

};


