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

#ifndef _UINAMEENTRY_H_
#define _UINAMEENTRY_H_

#include "UI.h"
#include "Timer.h"
#include "config.h"

namespace Zap
{


class TextEntryUserInterface : public UserInterface
{
   typedef UserInterface Parent;
private:
   U32 cursorPos;

protected:
   const char *title;
   const char *instr1;
   const char *instr2;
   
   bool resetOnActivate;
   LineEditor lineEditor;

   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;

public:
   explicit TextEntryUserInterface(ClientGame *game);  // Constructor
   virtual ~TextEntryUserInterface();                  // Destructor

   virtual void onActivate();
   void render();

   void setSecret(bool secret);

   virtual bool onKeyDown(InputCode inputCode);
   virtual void onTextInput(char ascii);

   string getText();
   void setString(string str);
};


////////////////////////////////////////
////////////////////////////////////////

class LevelNameEntryUserInterface : public TextEntryUserInterface
{

typedef TextEntryUserInterface Parent;

private:
   bool mFoundLevel;
   S32 mLevelIndex;
   Vector<string> mLevels;

   bool setLevelIndex();
   void completePartial();

public:
   explicit LevelNameEntryUserInterface(ClientGame *game);      // Constructor
   virtual ~LevelNameEntryUserInterface();

   virtual bool onKeyDown(InputCode inputCode);
   virtual void onAccept(const char *text);
   virtual void onEscape();

   virtual void onActivate();
};


////////////////////////////////////////
////////////////////////////////////////

class PasswordEntryUserInterface :  public TextEntryUserInterface
{
   typedef TextEntryUserInterface Parent;

public:
   explicit PasswordEntryUserInterface(ClientGame *game);    // Constructor
   virtual ~PasswordEntryUserInterface();

   virtual void render();
   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;
};


////////////////////////////////////////
////////////////////////////////////////

class GameConnection;

class ServerAccessPasswordEntryUserInterface :  public PasswordEntryUserInterface
{
   typedef PasswordEntryUserInterface Parent;

private:
   Address mConnectAddress;

public:
   explicit ServerAccessPasswordEntryUserInterface(ClientGame *game);    // Constructor
   virtual ~ServerAccessPasswordEntryUserInterface();
   void onAccept(const char *text);
   void onEscape();
   void setAddressToConnectTo(const Address &addr);
};


////////////////////////////////////////
////////////////////////////////////////

class LevelChangeOrAdminPasswordEntryUserInterface :  public PasswordEntryUserInterface
{
   typedef PasswordEntryUserInterface Parent;

public:
   explicit LevelChangeOrAdminPasswordEntryUserInterface(ClientGame *game);  // Constructor
   virtual ~LevelChangeOrAdminPasswordEntryUserInterface();

   void onAccept(const char *text);
   void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

class ServerPasswordEntryUserInterface : public ServerAccessPasswordEntryUserInterface
{
   typedef ServerAccessPasswordEntryUserInterface Parent;

public:
   explicit ServerPasswordEntryUserInterface(ClientGame *game);                // Constructor
   virtual ~ServerPasswordEntryUserInterface();
};


////////////////////////////////////////
////////////////////////////////////////

};

#endif

