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

#include "goalZone.h"
#include "gameType.h"
#include "gameObjectRender.h"
#include "game.h"
#include "stringUtils.h"


namespace Zap
{

TNL_IMPLEMENT_NETOBJECT(GoalZone);

/**
*  @luafunc GoalZone::GoalZone()
*  @luafunc GoalZone::GoalZone(polyGeom team)
*  @brief %GoalZone constructor.
*  @descr Default team is Neutral.
*/
// Combined Lua / C++ constructor
GoalZone::GoalZone(lua_State *L)
{
   mNetFlags.set(Ghostable);
   mObjectTypeNumber = GoalZoneTypeNumber;

   mFlashCount = 0;

   mHasFlag = false;
   mScore = 1;             // For now...  may someday let GoalZones have different scoring values
   mCapturer = NULL;
   setTeam(TEAM_NEUTRAL); 

   if(L)    // Coming from Lua -- grab params from L
   {
      static LuaFunctionArgList constructorArgList = { {{ END }, { POLY, TEAM_INDX, END }}, 2 };
      S32 profile = checkArgList(L, constructorArgList, "GoalZone", "constructor");

      if(profile == 1)        // Geom, Team
      {
         setGeom(L,  1);
         setTeam(L, -1);
      }
   }

   LUAW_CONSTRUCTOR_INITIALIZATIONS;
}
 

GoalZone::~GoalZone()
{
   LUAW_DESTRUCTOR_CLEANUP;
}


GoalZone *GoalZone::clone() const
{
   GoalZone *goalZone = new GoalZone(*this);
   return goalZone;
}


void GoalZone::render()
{
   GameType *gt = getGame()->getGameType();
   F32 glow = gt->mZoneGlowTimer.getFraction();

   // Check if to make sure that the zone matches the glow team if we're glowing
   if(gt->mGlowingZoneTeam >= 0 && gt->mGlowingZoneTeam != getTeam())
      glow = 0;

   bool useOldStyle = getGame()->getSettings()->getIniSettings()->oldGoalFlash;
   renderGoalZone(*getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle(), isFlashing(), glow, mScore, 
                  mFlashCount ? F32(mFlashTimer.getCurrent()) / FlashDelay : 0, useOldStyle);
}


void GoalZone::renderEditor(F32 currentScale, bool snappingToWallCornersEnabled)
{
   bool useOldStyle = getGame()->getSettings()->getIniSettings()->oldGoalFlash;

   renderGoalZone(*getColor(), getOutline(), getFill(), getCentroid(), getLabelAngle(), false, 0, 0, 0, useOldStyle);
   PolygonObject::renderEditor(currentScale, snappingToWallCornersEnabled);
}


void GoalZone::renderDock()
{
  renderGoalZone(*getColor(), getOutline(), getFill());
}


bool GoalZone::processArguments(S32 argc2, const char **argv2, Game *game)
{
   // Need to handle or ignore arguments that starts with letters,
   // so a possible future version can add parameters without compatibility problem.
   S32 argc = 0;
   const char *argv[65]; // 32 * 2 + 1 = 65
   for(S32 i = 0; i < argc2; i++)  // the idea here is to allow optional R3.5 for rotate at speed of 3.5
   {
      char c = argv2[i][0];

      if((c < 'a' || c > 'z') && (c < 'A' || c > 'Z'))
      {
         if(argc < 65)
         {  argv[argc] = argv2[i];
            argc++;
         }
      }
   }

   if(argc < 7)
      return false;

   setTeam(atoi(argv[0]));     // Team is first arg
   return Parent::processArguments(argc - 1, argv + 1, game);
}


const char *GoalZone::getOnScreenName()     { return "Goal";       }
const char *GoalZone::getOnDockName()       { return "Goal";       }
const char *GoalZone::getPrettyNamePlural() { return "Goal Zones"; }
const char *GoalZone::getEditorHelpString() { return "Target area used in a variety of games."; }

bool GoalZone::hasTeam()      { return true; }
bool GoalZone::canBeHostile() { return true; }
bool GoalZone::canBeNeutral() { return true; }


string GoalZone::toLevelCode(F32 gridSize) const
{
   return string(appendId(getClassName())) + " " + itos(getTeam()) + " " + geomToLevelCode(gridSize);
}


bool GoalZone::didRecentlyChangeTeam()
{
   return mFlashCount != 0;
}


void GoalZone::setTeam(S32 team)
{
   Parent::setTeam(team);
   setMaskBits(TeamMask);
}


// This just here to provide a signature at this level
void GoalZone::setTeam(lua_State *L, S32 index)
{
   Parent::setTeam(L, index);
}


ClientInfo *GoalZone::getCapturer()
{
   return mCapturer;
}


void GoalZone::setCapturer(ClientInfo *clientInfo)
{
   mCapturer = clientInfo;
}


void GoalZone::onAddedToGame(Game *theGame)
{
   Parent::onAddedToGame(theGame);

   if(!isGhost())
      setScopeAlways();
}


const Vector<Point> *GoalZone::getCollisionPoly() const
{
   return getOutline();
}


bool GoalZone::collide(BfObject *hitObject)
{
   if( !isGhost() && (isShipType(hitObject->getObjectTypeNumber())) )
   {
      Ship *s = static_cast<Ship *>(hitObject); 
      getGame()->getGameType()->shipTouchZone(s, this);
   }

   return false;
}


bool GoalZone::isFlashing()
{
   return mFlashCount & 1;
}


void GoalZone::setFlashCount(S32 i)
{
   mFlashCount = i;
}


S32 GoalZone::getScore()
{
   return mScore;
}


void GoalZone::setHasFlag(bool hasFlag)
{
   mHasFlag = hasFlag;
}


U32 GoalZone::packUpdate(GhostConnection *connection, U32 updateMask, BitStream *stream)
{
   Parent::packUpdate(connection, updateMask, stream);      // Handles Geomand Team
   
   if(stream->writeFlag(updateMask & InitialMask))
      stream->write(mScore);

   return 0;
}


void GoalZone::unpackUpdate(GhostConnection *connection, BitStream *stream)
{
   S32 oldTeam = getTeam();

   Parent::unpackUpdate(connection, stream);

   if(stream->readFlag()) 
      stream->read(&mScore);

   // Some special handling if we've changed teams
   if(getTeam() != oldTeam && !isInitialUpdate() && getTeam() != TEAM_NEUTRAL)   // Team will be neutral on touchdown, and we don't want to flash then!
   {
      mFlashTimer.reset(FlashDelay);
      mFlashCount = FlashCount;
   }
}


void GoalZone::idle(BfObject::IdleCallPath path)
{
   if(path != BfObject::ClientIdleMainRemote || mFlashCount == 0)
      return;

   if(mFlashTimer.update(mCurrentMove.time))
   {
      mFlashTimer.reset(FlashDelay);
      mFlashCount--;
   }
}


/////
// Lua interface
/**
  *  @luaclass GoalZone
  *  @brief Place to deposit flags or get the ball to, depending on game type.
  */
//               Fn name       Param profiles  Profile count                           
#define LUA_METHODS(CLASS, METHOD) \
   METHOD(CLASS, hasFlag,     ARRAYDEF({{ END }}), 1 ) \

GENERATE_LUA_METHODS_TABLE(GoalZone, LUA_METHODS);
GENERATE_LUA_FUNARGS_TABLE(GoalZone, LUA_METHODS);

#undef LUA_METHODS

const char *GoalZone::luaClassName = "GoalZone";
REGISTER_LUA_SUBCLASS(GoalZone, Zone);


/**
  *  @luafunc bool GoalZone::hasFlag()
  *  @brief Does the zone have a flag?
  *  @descr GoalZones can hold flags in some game types.  If the current game type does not feature
  *         zones that hold flags (e.g. Soccer, ZoneControl), then the function will return false.
  *  @return \e bool - True if the %GoalZone is currently holding a flag, false otherwise.
  */
S32 GoalZone::hasFlag(lua_State *L) { return returnBool(L, mHasFlag); }

};


