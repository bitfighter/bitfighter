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

#include "EditorObject.h"
#include "engineeredObjects.h"   // For Turret properties


inline F32 getGridSize()
{
   return gEditorGame->getGridSize();
}

// Except for commented lines, this is the same as GameObject's addtoEditor; can probably be merged
void EditorObject::addToEditor(Game *game)
{
   TNLAssert(mGame == NULL, "Error: Object already in a game in GameObject::addToGame.");
   TNLAssert(game != NULL,  "Error: theGame is NULL in GameObject::addToGame.");

   game->addToGameObjectList(this);
   //mCreationTime = theGame->getCurrentTime();
   mGame = game;
   addToDatabase();
   //onAddedToGame(game);
   gEditorUserInterface.mItems.push_back(this);
}


void EditorObject::addToDock(Game *game)
{
   mGame = game;
   mDockItem = true;
   gEditorUserInterface.mDockItems.push_back(this);
}


void EditorObject::processEndPoints()
{
   if(getObjectTypeMask() & BarrierType)
      Barrier::constructBarrierEndPoints(mVerts, getWidth() / getGridSize(), extendedEndPoints);

   else if(getObjectTypeMask() & PolyWallType)
   {
      extendedEndPoints.clear();
      for(S32 i = 1; i < getVertCount(); i++)
      {
         extendedEndPoints.push_back(mVerts[i-1]);
         extendedEndPoints.push_back(mVerts[i]);
      }

      // Close the loop
      extendedEndPoints.push_back(mVerts.last());
      extendedEndPoints.push_back(mVerts.first());
   }
}

const Color SELECT_COLOR = yellow;     // TODO: merge with UIEditor version

// Draw barrier centerlines; wraps renderPolyline()  ==> lineItem, barrierMaker only
void EditorObject::renderPolylineCenterline(F32 alpha)
{
   // Render wall centerlines
   if(mSelected)
      glColor(SELECT_COLOR, alpha);
   else if(mLitUp && !mAnyVertsSelected)
      glColor(EditorUserInterface::HIGHLIGHT_COLOR, alpha);
   else
      glColor(getTeamColor(mTeam), alpha);

   glLineWidth(WALL_SPINE_WIDTH);
   EditorUserInterface::renderPolyline(getVerts());
   glLineWidth(gDefaultLineWidth);
}


// Select a single vertex.  This is the default selection we use most of the time
void EditorObject::selectVert(S32 vertIndex) 
{ 
   unselectVerts();
   aselectVert(vertIndex);
}


// Select an additional vertex (remember command line ArcInfo?)
void EditorObject::aselectVert(S32 vertIndex)
{
   mVertSelected[vertIndex] = true;
   mAnyVertsSelected = true;
}


// Unselect a single vertex, considering the possibility that there may be other selected vertices as well
void EditorObject::unselectVert(S32 vertIndex) 
{ 
   mVertSelected[vertIndex] = false;

   bool anySelected = false;
   for(S32 j = 0; j < (S32)mVertSelected.size(); j++)
      if(mVertSelected[j])
      {
         anySelected = true;
         break;
      }
   mAnyVertsSelected = anySelected;
}


// Unselect all vertices
void EditorObject::unselectVerts() 
{ 
   for(S32 j = 0; j < getVertCount(); j++) 
      mVertSelected[j] = false; 
   mAnyVertsSelected = false;
}


bool EditorObject::vertSelected(S32 vertIndex) 
{ 
   return mVertSelected[vertIndex]; 
}


void EditorObject::addVert(const Point &vert)
{
   mVerts.push_back(vert);
   mVertSelected.push_back(false);
}


void EditorObject::addVertFront(Point vert)
{
   mVerts.push_front(vert);
   mVertSelected.insert(mVertSelected.begin(), false);
}


void EditorObject::insertVert(Point vert, S32 vertIndex)
{
   mVerts.insert(vertIndex);
   mVerts[vertIndex] = vert;

   mVertSelected.insert(mVertSelected.begin() + vertIndex, false);
}


void EditorObject::setVert(const Point &vert, S32 vertIndex)
{
   mVerts[vertIndex] = vert;
}


void EditorObject::deleteVert(S32 vertIndex)
{
   mVerts.erase(vertIndex);
   mVertSelected.erase(mVertSelected.begin() + vertIndex);
}


void EditorObject::onGeomChanging()
{
   if(getGeomType() == geomPoly)
      onGeomChanged();               // Allows poly fill to get reshaped as vertices move
}


// Item is being actively dragged
void EditorObject::onItemDragging()
{
   if(getObjectTypeMask() & ForceFieldProjectorType)
     onGeomChanged();

   else if(getGeomType() == geomPoly && getObjectTypeMask() & ~PolyWallType)
      onGeomChanged();     // Allows poly fill to get dragged around with outline
}



//void EditorObject::onGeomChanged()
//{
//   // TODO: Delegate all this to the member objects
//   if(getObjectTypeMask() & ItemBarrierMaker || getObjectTypeMask() & ItemPolyWall)
//   {  
//      // Fill extendedEndPoints from the vertices of our wall's centerline, or from PolyWall edges
//      processEndPoints();
//
//      if(getObjectTypeMask() & ItemPolyWall)     // Prepare interior fill triangulation
//         initializePolyGeom();          // Triangulate, find centroid, calc extents
//
//      getWallSegmentManager()->computeWallSegmentIntersections(this);
//
//      gEditorUserInterface.recomputeAllEngineeredItems();      // Seems awfully lazy...  should only recompute items attached to altered wall
//
//      // But if we're doing the above, we don't need to bother with the below... unless we stop being lazy
//      //// Find any forcefields that might intersect our new wall segment and recalc them
//      //for(S32 i = 0; i < gEditorUserInterface.mItems.size(); i++)
//      //   if(gEditorUserInterface.mItems[i]->index == ItemForceField &&
//      //                           gEditorUserInterface.mItems[i]->getExtent().intersects(getExtent()))
//      //      gEditorUserInterface.mItems[i]->findForceFieldEnd();
//   }
//
//   else if(getObjectTypeMask() & ItemForceField)
//   {
//      findForceFieldEnd();    // Find the end-point of the projected forcefield
//   }
//
//   else if(getGeomType() == geomPoly)
//      initializePolyGeom();
//
//   if(getObjectTypeMask() & ItemNavMeshZone)
//      gEditorUserInterface.rebuildBorderSegs(getItemId());
//}


// TODO: Move this to forcefield
void EditorObject::findForceFieldEnd()
{
   // Load the corner points of a maximum-length forcefield into geom
   Vector<Point> geom;
   DatabaseObject *collObj;

   F32 scale = 1 / getGridSize();
   
   Point start = ForceFieldProjector::getForceFieldStartPoint(getVert(0), mAnchorNormal, scale);

   if(ForceField::findForceFieldEnd(getGridDatabase(), start, mAnchorNormal, scale, forceFieldEnd, &collObj))
      forceFieldEndSegment = dynamic_cast<WallSegment *>(collObj);
   else
      forceFieldEndSegment = NULL;

   ForceField::getGeom(start, forceFieldEnd, geom, scale);    
   setExtent(Rect(geom));
}



////////////////////////////////////////
////////////////////////////////////////

 bool EditorObject::hasTeam()
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return true;
      case ItemSoccerBall: return false;
      case ItemFlag: return true;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return true;
      case ItemRepair: return false;
      case ItemEnergy: return false;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return false;
      case ItemMine: return false;
      case ItemSpyBug: return true;
      case ItemResource: return false;
      case ItemLoadoutZone: return true;
      case ItemNexus: return false;
      case ItemSlipZone: return false;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return true;
      case ItemNavMeshZone: return false;
   }*/
   return true;
}


bool EditorObject::canBeNeutral()
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return true;
      case ItemSoccerBall: return false;
      case ItemFlag: return true;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return true;
      case ItemRepair: return false;
      case ItemEnergy: return false;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return false;
      case ItemMine: return true;
      case ItemSpyBug: return true;
      case ItemResource: return false;
      case ItemLoadoutZone: return true;
      case ItemNexus: return true;
      case ItemSlipZone: return true;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return true;
      case ItemNavMeshZone: return true;
   }*/
   return true;
}

bool EditorObject::canBeHostile()
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return false;
      case ItemSoccerBall: return false;
      case ItemFlag: return true;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return true;
      case ItemRepair: return false;
      case ItemEnergy: return false;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return false;
      case ItemMine: return true;
      case ItemSpyBug: return true;
      case ItemResource: return false;
      case ItemLoadoutZone: return true;
      case ItemNexus: return true;
      case ItemSlipZone: return true;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return true;
      case ItemNavMeshZone: return true;
   }*/
   return true;
}

GeomType EditorObject::getGeomType()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return geomPoint;
      case ItemSoccerBall: return geomPoint;
      case ItemFlag: return geomPoint;
      case ItemFlagSpawn: return geomPoint;
      case ItemBarrierMaker: return geomLine;
      case ItemPolyWall: return geomPoly;
      case ItemLineItem: return geomLine;
      case ItemTeleporter: return geomSimpleLine;
      case ItemRepair: return geomPoint;
      case ItemEnergy: return geomPoint;
      case ItemBouncyBall: return geomPoint;
      case ItemAsteroid: return geomPoint;
      case ItemAsteroidSpawn: return geomPoint;
      case ItemMine: return geomPoint;
      case ItemSpyBug: return geomPoint;
      case ItemResource: return geomPoint;
      case ItemLoadoutZone: return geomPoly;
      case ItemNexus: return geomPoly;
      case ItemSlipZone: return geomPoly;
      case ItemTurret: return geomPoint;
      case ItemForceField: return geomPoint;
      case ItemGoalZone: return geomPoly;
      case ItemNavMeshZone: return geomPoly;
   }*/
   return geomNone;
}

bool EditorObject::getHasRepop()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return false;
      case ItemSoccerBall: return false;
      case ItemFlag: return false;
      case ItemFlagSpawn: return true;
      case ItemBarrierMaker: return false;
      case ItemPolyWall: return false;
      case ItemLineItem: return false;
      case ItemRepair: return true;
      case ItemEnergy: return true;
      case ItemBouncyBall: return false;
      case ItemAsteroid: return false;
      case ItemAsteroidSpawn: return true;
      case ItemMine: return false;
      case ItemSpyBug: return false;
      case ItemResource: return false;
      case ItemLoadoutZone: return false;
      case ItemNexus: return false;
      case ItemSlipZone: return false;
      case ItemTurret: return true;
      case ItemForceField: return true;
      case ItemGoalZone: return false;
      case ItemNavMeshZone: return false;
   }*/
   return false;
}

const char *EditorObject::getEditorHelpString()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return "Location where ships start.  At least one per team is required. [G]";
      case ItemSoccerBall: return "Soccer ball, can only be used in Soccer games.";
      case ItemFlag: return "Flag item, used by a variety of game types.";
      case ItemFlagSpawn: return "Location where flags (or balls in Soccer) spawn after capture.";
      case ItemBarrierMaker: return "Run-of-the-mill wall item.";
      case ItemPolyWall: return "Polygon wall barrier; create linear walls with right mouse click.";
      case ItemLineItem: return "Decorative linework.";
      case ItemRepair:  return "Repairs damage to ships. [B]";
      case ItemEnergy: return "Restores energy to ships";
      case ItemBouncyBall: return "Bouncy object that floats around and gets in the way.";
      case ItemAsteroid: return "Shootable asteroid object.  Just like the arcade game.";
      case ItemAsteroidSpawn: return "Periodically spawns a new asteroid.";
      case ItemMine: return "Mines can be prepositioned, and are are \"hostile to all\". [M]";
      case ItemSpyBug: return "Remote monitoring device that shows enemy ships on the commander's map. [Ctrl-B]";
      case ItemResource: return "Small bouncy object that floats around and gets in the way.";
      case ItemLoadoutZone: return "Area to finalize ship modifications.  Each team should have at least one.";
      case ItemNexus: return "Area to bring flags in Hunter game.  Cannot be used in other games.";
      case ItemSlipZone: return "Not yet implemented.";
      case ItemTurret: return "Creates shooting turret.  Can be on a team, neutral, or \"hostile to all\". [Y]";
      case ItemForceField: return "Creates a force field that lets only team members pass. [F]";
      case ItemGoalZone: return "Target area used in a variety of games.";
      case ItemNavMeshZone: return "Creates navigational mesh zone for robots.";
   }*/
   return "blug";
}


bool EditorObject::getSpecial()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return true;          
      case ItemSoccerBall: return true;     
      case ItemFlagSpawn: return true;      
      case ItemBouncyBall: return true;     
      case ItemAsteroid: return true;       
      case ItemAsteroidSpawn: return true;  
      case ItemMine: return true;   		
      case ItemResource: return true;   	
   }*/
   return false;
}


const char *EditorObject::getPrettyNamePlural()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return "Spawn points";   	
      case ItemSoccerBall: return "Soccer balls";     	
      case ItemFlag: return "Flags";                  	
      case ItemFlagSpawn: return "Flag spawn points"; 	
      case ItemBarrierMaker: return "Barrier makers"; 	
      case ItemPolyWall: return "PolyWalls";          	
      case ItemLineItem: return "Decorative Lines";   	
      case ItemRepair: return "Repair items";         	
      case ItemEnergy: return "Energy items";         	
      case ItemBouncyBall: return "Test items";       	
      case ItemAsteroid: return "Asteroids";          	
      case ItemAsteroidSpawn: return "Asteroid spawn points";
      case ItemMine: return "Mines";                  	
      case ItemSpyBug: return "Spy bugs";             	
      case ItemResource: return "Resource items";     	
      case ItemLoadoutZone: return "Loadout zones";   	
      case ItemNexus: return "Nexus zones";           	
      case ItemSlipZone: return "Slip zones";         	
      case ItemTurret: return "Turrets";              	
      case ItemForceField: return "Force field projectors"; 	
      case ItemGoalZone: return "Goal zones";             	
      case ItemNavMeshZone: return "NavMesh Zones";       	
   }*/
   return "blug";
}


const char *EditorObject::getOnDockName()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return "Spawn";    		
      case ItemSoccerBall: return "Ball";     		
      case ItemFlag: return "Flag";     			
      case ItemFlagSpawn: return "FlagSpawn";		
      case ItemBarrierMaker: return "Wall";     		
      case ItemPolyWall: return "Wall";     			
      case ItemLineItem: return "LineItem"; 			
      case ItemRepair: return "Repair";    			
      case ItemEnergy: return "Enrg";     			
      case ItemBouncyBall: return "Test";     		
      case ItemAsteroid: return "Ast.";     			
      case ItemAsteroidSpawn: return "ASP";      		
      case ItemMine: return "Mine";     			
      case ItemSpyBug: return "Bug";      			
      case ItemResource: return "Res.";     			
      case ItemLoadoutZone: return "Loadout";  		
      case ItemNexus: return "Nexus";    			
      case ItemSlipZone: return "Slip Zone";			
      case ItemTurret: return "Turret";   			
      case ItemForceField: return "ForceFld"; 		
      case ItemGoalZone: return "Goal";     			
      case ItemNavMeshZone: return "NavMesh";  		
   }
*/
   return "blug";
}


const char *EditorObject::getOnScreenName()     
{
   /*switch(getObjectTypeMask()) {
      case ItemSpawn: return "Spawn";        			
      case ItemSoccerBall: return "Ball";         			
      case ItemFlag: return "Flag";         				
      case ItemFlagSpawn: return "FlagSpawn";    			
      case ItemBarrierMaker: return "Wall";         			
      case ItemPolyWall: return "Wall";         			
      case ItemLineItem: return "LineItem";     			
      case ItemRepair: return "Repair";      			
      case ItemEnergy: return "Energy";       			
      case ItemBouncyBall: return "Test Item";    			
      case ItemAsteroid: return "Asteroid";     			
      case ItemAsteroidSpawn: return "AsteroidSpawn";		
      case ItemMine: return "Mine";         				
      case ItemSpyBug: return "Spy Bug";      			
      case ItemResource: return "Resource";     			
      case ItemLoadoutZone: return "Loadout";      			
      case ItemNexus: return "Nexus";        			
      case ItemSlipZone: return "Slip Zone";    			
      case ItemTurret: return "Turret";       			
      case ItemForceField: return "ForceFld";     			
      case ItemGoalZone: return "Goal";         			
      case ItemNavMeshZone: return "NavMesh";      			
   }*/
   return "blug";
}

//
//const char *EditorObject::getName()     
//{
//   switch(getObjectTypeMask()) {			
//      case ItemSpawn: return "Spawn";        			
//      case ItemSpeedZone: return "SpeedZone";       	
//      case ItemSoccerBall: return "SoccerBallItem";    	
//      case ItemFlag: return "FlagItem";         		
//      case ItemFlagSpawn: return "FlagSpawn";    		
//      case ItemBarrierMaker: return "BarrierMaker";      	
//      case ItemPolyWall: return "PolyWall";         	
//      case ItemLineItem: return "LineItem";     		
//      case ItemTeleporter: return "Teleporter";     	
//      case ItemRepair: return "RepairItem";      		
//      case ItemEnergy: return "EnergyItem";       		
//      case ItemBouncyBall: return "TestItem";    		
//      case ItemAsteroid: return "Asteroid";     		
//      case ItemAsteroidSpawn: return "AsteroidSpawn";	
//      case ItemMine: return "Mine";         			  
//      case ItemSpyBug: return "SpyBug";      		 
//      case ItemResource: return "ResourceItem";     	
//      case ItemLoadoutZone: return "LoadoutZone";     
//      case ItemNexus: return "HuntersNexusObject";    
//      case ItemSlipZone: return "SlipZone";    		          	
//      case ItemTurret: return "Turret";       		             	
//      case ItemForceField: return "ForceFieldProjector"; 
//      case ItemGoalZone: return "GoalZone";         			
//      case ItemTextItem: return "TextItem";         			
//      case ItemNavMeshZone: return "BotNavMeshZone";     	
//   }return "blug";
//}
