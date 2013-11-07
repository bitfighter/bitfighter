//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _TEAMSHUFFLE_H_
#define _TEAMSHUFFLE_H_

#include "helperMenu.h"

#include "tnlVector.h"

using namespace TNL;

namespace Zap
{

class ClientInfo;

class TeamShuffleHelper : public HelperMenu
{
   typedef HelperMenu Parent;

private:
   const char *getCancelMessage() const;
   InputCode getActivationKey();

   S32 columnWidth;
   S32 maxColumnWidth;
   S32 rowHeight;
   S32 rows, cols;
   S32 teamCount;
   S32 playersPerTeam;

   S32 topMargin, leftMargin;

   static const S32 vpad = 10, hpad = 14;    // Padding inside the boxes
   static const S32 TEXT_SIZE = 22;
   static const S32 margin = 10;

   Vector<Vector<ClientInfo *> > mTeams;
   void shuffle();
   void calculateRenderSizes();

public:
   explicit TeamShuffleHelper();             // Constructor
   virtual ~TeamShuffleHelper();

   HelperMenuType getType();

   void render();                
   void onActivated();  

   bool processInputCode(InputCode inputCode);   

   bool isMovementDisabled() const;

   void onPlayerJoined();
   void onPlayerQuit();
};

};

#endif


