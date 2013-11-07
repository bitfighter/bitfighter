//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEXTITEM_H_
#define _TEXTITEM_H_

#include "SimpleLine.h"       // For base class def
#include "Color.h"

using namespace std;

namespace Zap
{

class ClientGame;
static const S32 MAX_TEXTITEM_LEN = 255;

class TextItem : public SimpleLine
{
   typedef SimpleLine Parent;

private:
   F32 mSize;              // Text size
   string mText;           // Text itself

#ifndef ZAP_DEDICATED
   static EditorAttributeMenuUI *mAttributeMenuUI;      // Menu for text editing; since it's static, don't bother with smart pointer
#endif

public:
   static const S32 MAX_TEXT_SIZE = 255;
   static const S32 MIN_TEXT_SIZE = 10;

   explicit TextItem(lua_State *L = NULL);   // Combined Lua / C++ constructor
   virtual ~TextItem();             // Destructor

   TextItem *clone() const;

   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv, Game *game);  // Create objects from parameters stored in level file
   string toLevelCode() const;
   void setGeom(const Vector<Point> &points);
   void setGeom(const Point &pos, const Point &dest);
   void setGeom(lua_State *L, S32 index);
   Rect calcExtents();      // Bounding box for display scoping purposes


   void onAddedToGame(Game *theGame);  

   static void textEditedCallback(string text, BfObject *obj);

   const Vector<Point> *getCollisionPoly() const;          // More precise boundary for precise collision detection
   bool collide(BfObject *hitObject);
   void idle(BfObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   ///// Editor Methods

   Color getEditorRenderColor();

   void renderEditorItem();
   F32 getSize();

   string getText();
   void setText(const string &text);
   void setText(lua_State *L, S32 index);

   void onAttrsChanging();
   void onAttrsChanged();
   void onGeomChanging();
   void onGeomChanged();

   void recalcTextSize();
   void setSize(F32 desiredSize);


   // Some properties about the item that will be needed in the editor
   const char *getEditorHelpString();
   const char *getPrettyNamePlural();
   const char *getOnDockName();
   const char *getOnScreenName();
   const char *getInstructionMsg(S32 attributeCount);
   bool hasTeam();
   bool canBeHostile();
   bool canBeNeutral();

   void fillAttributesVectors(Vector<string> &keys, Vector<string> &values);

   void newObjectFromDock(F32 gridSize);

   TNL_DECLARE_CLASS(TextItem);

   //// Lua interface
   LUAW_DECLARE_CLASS_CUSTOM_CONSTRUCTOR(TextItem);

	static const char *luaClassName;
	static const luaL_reg luaMethods[];
   static const LuaFunctionProfile functionArgs[];

   S32 lua_setText(lua_State *L);
   S32 lua_getText(lua_State *L);
};


};

#endif


