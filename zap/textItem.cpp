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

#include "textItem.h"
#include "UIEditorMenus.h"       // For TextItemEditorAttributeMenuUI def
#include "gameNetInterface.h"
#include "glutInclude.h"
#include "gameObjectRender.h"    // For renderPointVector()

namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(TextItem);

#ifndef TNL_OS_WIN32
const U32 TextItem::MAX_TEXT_SIZE;
const U32 TextItem::MIN_TEXT_SIZE;
#endif


EditorAttributeMenuUI *TextItem::mAttributeMenuUI = NULL;


// Constructor
TextItem::TextItem()
{
   mNetFlags.set(Ghostable);
   mObjectTypeMask = TextItemType | CommandMapVisType;     // Or not?

   // Some default values
   mSize = 20;
   mTeam = Item::TEAM_NEUTRAL;
}


// Destructor
TextItem::~TextItem()
{
   // Do nothing
}


void TextItem::initializeEditor(F32 gridSize)
{
   SimpleLine::initializeEditor(gridSize);

   mText = "Your text here";
   recalcTextSize();
}


// In game rendering
void TextItem::render()
{
   ClientGame *game = dynamic_cast<ClientGame *>(getGame());
   
   // Don't render opposing team's text items
   if(!game || !game->getConnectionToServer() || !game->getGameType())
      return;

   Ship *ship = dynamic_cast<Ship *>(game->getConnectionToServer()->getControlObject());
   if( (!ship && mTeam != -1) || (ship && ship->getTeam() != mTeam && mTeam != -1) )
      return;

   renderTextItem(mPos, mDir, mSize, mText, game->getGameType()->getTeamColor(mTeam));
}


// Called by SimpleItem::renderEditor()
void TextItem::renderEditorItem()
{
   renderTextItem(mPos, mDir, mSize, mText, getGame()->getTeamColor(mTeam));
}


// This object should be drawn below others
S32 TextItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// Entry looks like: TextItem 0 50 10 10 11 11 Message goes here
bool TextItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 7)
      return false;

   mTeam = atoi(argv[0]);

   mPos.read(argv + 1);
   mPos *= getGame()->getGridSize();

   mDir.read(argv + 3);
   mDir *= getGame()->getGridSize();

   mSize = atof(argv[5]);
   mSize = max(min(mSize, MAX_TEXT_SIZE), MIN_TEXT_SIZE);      // Note that same line exists below, in recalcXXX()... combine?

   // Assemble any remainin args into a string
   mText = "";
   for(S32 i = 6; i < argc; i++)
   {
      mText += argv[i];
      if (i < argc - 1)
         mText += " ";
   }

   computeExtent();

   return true;
}


string TextItem::toString()
{
   F32 gs = getGame()->getGridSize();

   char outString[LevelLoader::MAX_LEVEL_LINE_LENGTH];
   dSprintf(outString, sizeof(outString), "%s %d %g %g %g %g %g %s", Object::getClassName(), mTeam, 
                                          mPos.x / gs, mPos.y / gs, mDir.x / gs, mDir.y / gs, mSize, mText.c_str());
   return outString;
}


// Editor
void TextItem::recalcTextSize()
{
   const F32 dummyTextSize = 120;

   F32 lineLen = getVert(0).distanceTo(getVert(1));      // In in-game units
   F32 strWidth = F32(UserInterface::getStringWidth(dummyTextSize, mText.c_str())) / dummyTextSize; 
   F32 size = lineLen / strWidth;

   // Compute text size subject to min and max defined in TextItem
   mSize = max(min(size, MAX_TEXT_SIZE), MIN_TEXT_SIZE);
}


void TextItem::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}


// Bounding box for quick collision-possibility elimination, and display scoping purposes
void TextItem::computeExtent()
{
   U32 len = UserInterface::getStringWidth(mSize, mText.c_str());
   U32 buf = mSize / 2;     // Provides some room to accomodate descenders on letters like j and g.

   F32 angle =  mPos.angleTo(mDir);
   F32 sinang = sin(angle);
   F32 cosang = cos(angle);

   F32 descenderFactor = .35;    // To account for y, g, j, etc.
   F32 h = mSize * (1 + descenderFactor);
   F32 w = len * 1.05;           // 1.05 adds just a little horizontal padding for certain words with trailing ys or other letters that are just a tiny bit longer than calculated
   F32 x = mPos.x + mSize * descenderFactor * sinang;
   F32 y = mPos.y + mSize * descenderFactor * cosang;

   F32 c1x = x - h * sinang * .5;
   F32 c1y = y;

   F32 c2x = x + w * cosang - h * sinang * .5;
   F32 c2y = y + w * sinang;

   F32 c3x = x + h * sinang * .5 + w * cosang;
   F32 c3y = y - h * cosang + w * sinang;

   F32 c4x = x + h * sinang * .5;
   F32 c4y = y - h * cosang;

   F32 minx = min(c1x, min(c2x, min(c3x, c4x)));
   F32 miny = min(c1y, min(c2y, min(c3y, c4y)));
   F32 maxx = max(c1x, max(c2x, max(c3x, c4x)));
   F32 maxy = max(c1y, max(c2y, max(c3y, c4y)));

   Rect extent(Point(minx, miny), Point(maxx, maxy));

   setExtent(extent);
}


bool TextItem::getCollisionPoly(Vector<Point> &polyPoints)
{
   return false;
}

// Handle collisions with a TextItem.  Easy, there are none.
bool TextItem::collide(GameObject *hitObject)
{
   return false;
}

void TextItem::idle(GameObject::IdleCallPath path)
{
   // Laze about, read a book, take a nap, whatever.
}

U32 TextItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   stream->write(mPos.x);
   stream->write(mPos.y);

   stream->write(mDir.x);
   stream->write(mDir.y);

   stream->writeRangedU32(mSize, 0, MAX_TEXT_SIZE);
   stream->write(mTeam);

   stream->writeString(mText.c_str(), (U8) mText.length());      // Safe to cast text.length to U8 because we've limited it's length to MAX_TEXTITEM_LEN

   return 0;
}

void TextItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   char txt[MAX_TEXTITEM_LEN];

   stream->read(&mPos.x);
   stream->read(&mPos.y);

   stream->read(&mDir.x);
   stream->read(&mDir.y);

   mSize = stream->readRangedU32(0, MAX_TEXT_SIZE);
   stream->read(&mTeam);

   stream->readString(txt);

   mText = txt;
   computeExtent();
}


///// Editor Methods

// Runs when text is being changed in the editor
void TextItem::onAttrsChanging()
{
   onGeomChanged();
}

void TextItem::onAttrsChanged()
{
   onGeomChanged();
}

void TextItem::onGeomChanging()
{
   onGeomChanged();
}

void TextItem::onGeomChanged()
{
   recalcTextSize();
}


// Static method: Provide hook into the object currently being edited with the attrubute editor for callback purposes
EditorObject *TextItem::getAttributeEditorObject()     
{ 
   return mAttributeMenuUI->getObject(); 
}


EditorAttributeMenuUI *TextItem::getAttributeMenu()
{
   // Lazily initialize this -- if we're in the game, we'll never need this to be instantiated
   if(!mAttributeMenuUI)
      mAttributeMenuUI = new TextItemEditorAttributeMenuUI;

   // Udate the editor with attributes from our current object
   mAttributeMenuUI->menuItems[0]->setValue(mText);

   return mAttributeMenuUI;
}


////////////////////////////////////////
////////////////////////////////////////

TNL_IMPLEMENT_NETOBJECT(LineItem);

// Constructor
LineItem::LineItem()
{ 
   mNetFlags.set(Ghostable);
   mObjectTypeMask |= CommandMapVisType;
}


void LineItem::render()
{
   // Don't render opposing team's text items
   if(!gClientGame || !gClientGame->getConnectionToServer())      // Not sure if this is really needed...
      return;

   Ship *ship = dynamic_cast<Ship *>(gClientGame->getConnectionToServer()->getControlObject());
   if( (!ship && mTeam != -1) || (ship && ship->getTeam() != mTeam && mTeam != -1) )
      return;

   glColor(getGame()->getTeamColor(mTeam));
   renderPointVector(mPolyBounds, GL_LINE_STRIP);
}


void LineItem::renderEditor(F32 currentScale)
{
   glColor(getGame()->getTeamColor(mTeam));
   renderPointVector(mPolyBounds, GL_LINE_STRIP);

   renderLinePolyVertices(currentScale);
}


// This object should be drawn below others
S32 LineItem::getRenderSortValue()
{
   return 1;
}


// Create objects from parameters stored in level file
// Entry looks like: LineItem 0 50 10 10 11 11 Message goes here
bool LineItem::processArguments(S32 argc, const char **argv)
{
   if(argc < 6)
      return false;

   mTeam = atoi(argv[0]);
   setWidth(max(min(atoi(argv[1]), MAX_LINE_WIDTH), MIN_LINE_WIDTH));
   processPolyBounds(argc, argv, 2, getGame()->getGridSize());

   computeExtent();

   return true;
}


string LineItem::toString()
{
   return string(getClassName()) + " " + itos(mTeam) + " " + itos(getWidth()) + " " + boundsToString(getGame()->getGridSize());
}


void LineItem::onAddedToGame(Game *theGame)
{
   if(!isGhost())
      setScopeAlways();

   getGame()->mObjectsLoaded++;
}


// Bounding box for quick collision-possibility elimination, and display scoping purposes
void LineItem::computeExtent()
{
   setExtent(computePolyExtents());
}


bool LineItem::getCollisionPoly(Vector<Point> &polyPoints)
{
   return false;
}


// Handle collisions with a LineItem.  Easy, there are none.
bool LineItem::collide(GameObject *hitObject)
{
   return false;
}


void LineItem::idle(GameObject::IdleCallPath path)
{
   // Do nothing
}


U32 LineItem::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   //stream->writeRangedU32(mWidth, 0, MAX_LINE_WIDTH);
   stream->write(mTeam);

   Polyline::packUpdate(connection, stream);

   return 0;
}


void LineItem::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   //mWidth = stream->readRangedU32(0, MAX_LINE_WIDTH);
   stream->read(&mTeam);

   Polyline::unpackUpdate(connection, stream);

   setExtent(computePolyExtents());
}


////////////////////////////////////////
////////////////////////////////////////

// Constructor
WallItem::WallItem()
{
   mObjectTypeMask = BarrierType;
   setWidth(50 * 256);
}


void WallItem::onGeomChanged()
{
   // Fill extendedEndPoints from the vertices of our wall's centerline, or from PolyWall edges
   processEndPoints();

   //if(getObjectTypeMask() & ItemPolyWall)     // Prepare interior fill triangulation
   //   initializePolyGeom();          // Triangulate, find centroid, calc extents

   gEditorUserInterface.getWallSegmentManager()->computeWallSegmentIntersections(this);

   gEditorUserInterface.recomputeAllEngineeredItems();      // Seems awfully lazy...  should only recompute items attached to altered wall

   // But if we're doing the above, we don't need to bother with the below... unless we stop being lazy
   //// Find any forcefields that might intersect our new wall segment and recalc them
   //for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
   //   if(gEditorUserInterface.mItems[i]->index == ItemForceField &&
   //                           gEditorUserInterface.mItems[i]->getExtent().intersects(getExtent()))
   //      gEditorUserInterface.mItems[i]->findForceFieldEnd();
}


string WallItem::toString()
{
   return "BarrierMaker " + itos(S32(F32(mWidth) / F32(getGame()->getGridSize()))) + " " + boundsToString(getGame()->getGridSize());
}

};


