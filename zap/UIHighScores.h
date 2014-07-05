//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _UI_HIGH_SCORES_H_
#define _UI_HIGH_SCORES_H_

#include "UI.h"                  // Parent class
#include "tnlNetStringTable.h"   // For StringTableEntry def


namespace Zap
{

using namespace std;


class HighScoresUserInterface : public UserInterface
{
   typedef UserInterface Parent;

private:
   bool mHaveHighScores;

   void renderScores() const;
   void renderWaitingForScores() const;

   struct ScoreGroup {
      string title;
      Vector<string> names;
      Vector<string> scores;
   };

   Vector<ScoreGroup> mScoreGroups;

public:
   explicit HighScoresUserInterface(ClientGame *game);    // Constructor
   virtual ~HighScoresUserInterface();

   void onActivate();
   void onReactivate();
   void render() const;
   void idle(U32 timeDelta);

   void setHighScores(Vector<StringTableEntry> groupNames, Vector<string> names, Vector<string> scores);

   bool onKeyDown(InputCode inputCode);
   void quit();

};

};

#endif

