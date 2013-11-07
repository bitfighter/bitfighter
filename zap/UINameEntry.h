//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

   F32 getFontSize();


   virtual void onAccept(const char *text) = 0;
   virtual void onEscape() = 0;

public:
   explicit TextEntryUserInterface(ClientGame *game);  // Constructor
   virtual ~TextEntryUserInterface();                  // Destructor

   virtual void onActivate();
   virtual void render();

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

   void render();
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

