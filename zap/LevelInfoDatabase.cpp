//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "LevelInfoDatabase.h"

#include "gameType.h"
#include "stringUtils.h"

#include <sstream>
#include <iostream>
#include <fstream>
   

using namespace std;
using namespace TNL;
using namespace Zap;


namespace LevelInfoDb
{

// Adapted from http://stackoverflow.com/questions/17738992/c-csv-parsing-with-commas-inside-of-quotes

// Assume "in" is positioned at start of column.
// Accumulates chars from "in" as long as "in" is good.
// Loops until either:
//    1. We've consumed a comma preceded by 0 quotes; or
//    2. We've consumed a comma immediately preceded by the last of an even number of quotes
// Should clean the resulting value:
//    1. Remove trailing ,
//    2. Trim any whitespace
//    3. Remove quotes used to quote the value

static string getNextCsvColumn(istream &in)
{
   string col;
   S32 quotes = 0;
   char ch, prev = 0;
   bool hasLeadingQuote = false;    // True if this looks like a quoted value (i.e. starts with ")

   while((ch = in.get()) != EOF) 
   {
      switch(ch) 
      {
         case ',':
            if(quotes == 0 || (prev == '"' && (quotes & 1) == 0)) 
               goto done;

            col += prev = ch;
            break;

         case ' ':
            if(prev != 0)  // If prev == 0, we're still munching leading whitespace and should not save the char
               col += prev = ch;
            break;

         case '"':
            quotes++;
            if(prev == 0)
            {
               hasLeadingQuote = true;
               prev = ch;
            }
            else
               col += prev = ch;

            break;

         default:
            col += prev = ch;
      }      
   }

done:
   col = Zap::trim(col);

   // If we found a quote at the beginning of the value, chop it from the end, if it's there
   if(hasLeadingQuote && col[col.length() - 1] == '"')
      col.erase(col.length() - 1);

   return col;
}


void readCsvFromStream(istream &stream, LevelInfoDatabase &levelDb)
{
   static enum Cols {
      Md5Col,
      NameCol,
      GameTypeCol,
      MinPlayersCol,
      MaxPlayersCol,
      LevelSizeCol,
      ScriptNameCol,
      ColsExpected
   };


   string line, phrase, word, md5;
   Vector<string> words;

   while(getline(stream, line)) 
   {
      stringstream lineStream(line);
      words.clear();

      while(lineStream)
        words.push_back(getNextCsvColumn(lineStream));

      if(words.size() == ColsExpected)
      {
         // Check for a valid gameType -- returns NoGameType if passed string is bogus
         GameTypeId gameTypeId = GameType::getGameTypeIdFromName(words[GameTypeCol]);
         if(gameTypeId == NoGameType)
            continue;

         levelDb.levelName[words[Md5Col]] = words[NameCol];
         levelDb.gameTypeId[words[Md5Col]] = gameTypeId;
         levelDb.minPlayers[words[Md5Col]] = atoi(words[MinPlayersCol].c_str());
         levelDb.maxPlayers[words[Md5Col]] = atoi(words[MaxPlayersCol].c_str());
         levelDb.levelSize [words[Md5Col]] = atoi(words[LevelSizeCol].c_str());
         levelDb.scriptName[words[Md5Col]] = words[ScriptNameCol].c_str();
      }
   }
}


bool readCsv(const string &filename)
{
   LevelInfoDatabase levelDb;

   ifstream file(filename);
   if(!file)
      return false;

   readCsvFromStream(file, levelDb);

   file.close();
   return true;
}


static const string LevelInfoDatabaseFilename = "levelinfo.csv";

void saveLevelInfo(const string &hash, const LevelInfo &levelInfo)
{
  ofstream outfile;

  outfile.open(LevelInfoDatabaseFilename, std::ios_base::app);
  levelInfo.writeToStream(outfile, hash);
}


}