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

   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;
   const char *getText() { return lineEditor.c_str(); }
   void setString(const char *text);
   virtual bool isValid(char ascii);      // Ensure only valid characters are entered
};

////////////////


class NameEntryUserInterface : public TextEntryUserInterface
{
public:
   NameEntryUserInterface();      // Constructor

   virtual void onAccept(const char *text);
   virtual void onEscape();
};

extern NameEntryUserInterface gNameEntryUserInterface;

////////////////

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
   }
   virtual void onAccept(const char *text);
   virtual void onEscape();
   bool isValid(char ascii);
};

extern LevelNameEntryUserInterface gLevelNameEntryUserInterface;

////////////////

class PasswordEntryUserInterface : public TextEntryUserInterface
{
   Address connectAddress;
public:
   PasswordEntryUserInterface()        // Constructor
   {
      setMenuID(PasswordEntryUI);
      title = "ENTER SERVER PASSWORD:";
      instr1 = "";
      instr2 = "Enter the password required for access to the server";
      secret = true;
   }
   void onAccept(const char *text);
   void onEscape();
   void setConnectServer(const Address &addr) { connectAddress = addr; }
};

extern PasswordEntryUserInterface gPasswordEntryUserInterface;

////////////////

class ReservedNamePasswordEntryUserInterface : public TextEntryUserInterface
{
   Address connectAddress;
public:
   ReservedNamePasswordEntryUserInterface()        // Constructor
   {
      setMenuID(ReservedNamePasswordEntryUI);
      title = "ENTER USERNAME PASSWORD:";
      instr1 = "The username you are using has been reserved on this server.";
      instr2 = "Please enter the password to use this name here.";
      secret = true;
   }
   void onAccept(const char *text);
   void onEscape();
   void setConnectServer(const Address &addr) { connectAddress = addr; }
};

extern ReservedNamePasswordEntryUserInterface gReservedNamePasswordEntryUserInterface;

////////////////

class AdminPasswordEntryUserInterface : public TextEntryUserInterface
{
   typedef TextEntryUserInterface Parent;
public:
   AdminPasswordEntryUserInterface()      // Constructor
   {
      setMenuID(AdminPasswordEntryUI);
      title = "ENTER ADMIN PASSWORD:";
      instr1 = "";
      instr2 = "Enter the admin password to perform admin tasks and change levels on this server";
      secret = true;
   }
   void onAccept(const char *text);
   void onEscape();
   void render();
};

extern AdminPasswordEntryUserInterface gAdminPasswordEntryUserInterface;

////////////////

class LevelChangePasswordEntryUserInterface : public TextEntryUserInterface
{
   typedef TextEntryUserInterface Parent;
public:
   LevelChangePasswordEntryUserInterface()      // Constructor
   {
      setMenuID(LevelChangePasswordEntryUI);
      title = "ENTER LEVEL CHANGE PASSWORD:";
      instr1 = "";
      instr2 = "Enter the level change password to change levels on this server";
      secret = true;
   }
   void onAccept(const char *text);
   void onEscape();
   void render();
};

extern LevelChangePasswordEntryUserInterface gLevelChangePasswordEntryUserInterface;

////////////////

};

#endif

