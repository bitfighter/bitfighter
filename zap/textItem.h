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

#ifndef _TEXTITEM_H_
#define _TEXTITEM_H_


#include "gameObject.h"
#include "gameType.h"
#include "gameNetInterface.h"
#include "UI.h"
#include "gameObjectRender.h"
#include "ship.h"
#include "../glut/glutInclude.h"
#include <string>

//TODO: Make these regular vars
#define MAX_TEXTITEM_LEN 255
#define MAX_TEXT_SIZE 255
#define MIN_TEXT_SIZE 10

using namespace std;

namespace Zap
{

class TextItem : public GameObject
{
private:

   typedef GameObject Parent;

public:

   Point pos;            // Location of text
   Point dir;            // Direction text is "facing"
   U32 mSize;            // Text size
   S32 mTeam;            // Team text is visible to (-1 for visible to all)
   string mText;         // Text iteself
   
   TextItem();   // Constructor

   static Vector<Point> generatePoints(Point pos, Point dir);
   void render();
   S32 getRenderSortValue();

   bool processArguments(S32 argc, const char **argv);           // Create objects from parameters stored in level file
   void onAddedToGame(Game *theGame);
   void computeExtent();                                         // Bounding box for quick collision-possibility elimination

   bool getCollisionPoly(Vector<Point> &polyPoints);  // More precise boundary for precise collision detection
   bool collide(GameObject *hitObject);
   void idle(GameObject::IdleCallPath path);
   U32 packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream);
   void unpackUpdate(GhostConnection *connection, BitStream *stream);

   TNL_DECLARE_CLASS(TextItem);
};


};

#endif


