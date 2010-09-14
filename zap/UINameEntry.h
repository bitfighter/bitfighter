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
#include "timer.h"
#include "config.h"

namespace Zap
{

class TextEntryUserInterface : public UserInterface
{
private:
   U32 cursorPos;

protected:
   const char *title;
   const char *instr1;
   const char *instr2;
   bool secret;
   bool resetOnActivate;
   LineEditor lineEditor;

   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;

public:
   TextEntryUserInterface()   // Constructor
   {
      setMenuID(TextEntryUI);
      title = "ENTER TEXT:";
      instr1 = "";
      instr2 = "Enter some text above";
      secret = false;
      cursorPos = 0;
      resetOnActivate = true;
      lineEditor = LineEditor(MAX_SHORT_TEXT_LEN);
   }

   void onActivate();
   void render();
   void idle(U32 t);

   void onKeyDown(KeyCode keyCode, char ascii);

   const char *getText() { return lineEditor.c_str(); }
   void setString(string str);
};


////////////////////////////////////////
////////////////////////////////////////

class NameEntryUserInterface : public TextEntryUserInterface
{
public:
   NameEntryUserInterface();      // Constructor

   virtual void onAccept(const char *text);
   virtual void onEscape();
};


////////////////////////////////////////
////////////////////////////////////////

extern NameEntryUserInterface gNameEntryUserInterface;
extern IniSettings gIniSettings;

class LevelNameEntryUserInterface : public TextEntryUserInterface
{
public:
   LevelNameEntryUserInterface()       // Constructor
   {
      setMenuID(LevelNameEntryUI);
      title = "ENTER LEVEL TO EDIT:";
      instr1 = "";
      instr2 = "Enter an existing level, or create your own!";
      resetOnActivate = false;
      lineEditor.setFilter(LineEditor::fileNameFilter);
   }

   virtual void onAccept(const char *text);
   virtual void onEscape();
};

extern LevelNameEntryUserInterface gLevelNameEntryUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class PasswordEntryUserInterface :  public TextEntryUserInterface
{
   typedef TextEntryUserInterface Parent;

public:
   PasswordEntryUserInterface()     // Constructor
   {
      secret = true;
   }

   virtual void render();
   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;
};


////////////////////////////////////////
////////////////////////////////////////

class GameConnection;

class PreGamePasswordEntryUserInterface :  public PasswordEntryUserInterface
{
private:
   Address connectAddress;
public:
   void onAccept(const char *text);
   void onEscape();
   void setConnectServer(const Address &addr) { connectAddress = addr; }
};


////////////////////////////////////////
////////////////////////////////////////

class InGamePasswordEntryUserInterface :  public PasswordEntryUserInterface
{
public:
   void onAccept(const char *text);
   void onEscape();
   virtual void submitPassword(GameConnection *gameConnection, const char *text) = 0;
};


////////////////////////////////////////
////////////////////////////////////////

class ServerPasswordEntryUserInterface : public PreGamePasswordEntryUserInterface
{
public:
   ServerPasswordEntryUserInterface();                // Constructor
};

extern ServerPasswordEntryUserInterface gServerPasswordEntryUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class ReservedNamePasswordEntryUserInterface : public PreGamePasswordEntryUserInterface
{
public:
   ReservedNamePasswordEntryUserInterface();        // Constructor
};

extern ReservedNamePasswordEntryUserInterface gReservedNamePasswordEntryUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class AdminPasswordEntryUserInterface : public InGamePasswordEntryUserInterface
{
public:
   AdminPasswordEntryUserInterface();           // Constructor
   void submitPassword(GameConnection *gameConnection, const char *text);
};

extern AdminPasswordEntryUserInterface gAdminPasswordEntryUserInterface;


////////////////////////////////////////
////////////////////////////////////////

class LevelChangePasswordEntryUserInterface : public InGamePasswordEntryUserInterface
{
public:
   LevelChangePasswordEntryUserInterface();     // Constructor
   void submitPassword(GameConnection *gameConnection, const char *text);
};

extern LevelChangePasswordEntryUserInterface gLevelChangePasswordEntryUserInterface;

////////////////////////////////////////
////////////////////////////////////////

};

#endif

