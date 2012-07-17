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
#include "WallSegmentManager.h"
#include "EngineeredItem.h"      // For Turret properties
#include "soccerGame.h"          // For soccer ball radius

#include "textItem.h"            // For copy constructor
#include "teleporter.h"          // For copy constructor
#include "speedZone.h"           // For copy constructor
#include "loadoutZone.h"
#include "goalZone.h"
#include "NexusGame.h"
#include "Colors.h"
#include "game.h"
#include "config.h"

#include "Geometry.h"            // For GeomType enum
#include "stringUtils.h"

#ifndef ZAP_DEDICATED
#  include "ClientGame.h"       // For ClientGame and getUIManager()
#  include "UIEditorMenus.h"    // For EditorAttributeMenuUI def
#  ifdef TNL_OS_MOBILE
#     include "SDL_opengles.h"
#  else
#     include "SDL_opengl.h"
#  endif
#endif

using namespace boost;

namespace Zap
{
 
// TODO: merge with simpleLine values, put in editor
static const S32 INSTRUCTION_TEXTSIZE = 12;      
static const S32 INSTRUCTION_TEXTGAP = 4;
static const Color INSTRUCTION_TEXTCOLOR = Colors::white;      // TODO: Put in editor


// Constructor
PointObject::PointObject()
{
   setNewGeometry(geomPoint);
}


// Destructor
PointObject::~PointObject()
{
   // Do nothing
}


void PointObject::setGeom(const Vector<Point> &points)
{
   if(points.size() > 0)
      setPos(points[0]);
}


void PointObject::prepareForDock(ClientGame *game, const Point &point, S32 teamIndex)
{
#ifndef ZAP_DEDICATED
   setPos(point);
   Parent::prepareForDock(game, point, teamIndex);
#endif
}

};
