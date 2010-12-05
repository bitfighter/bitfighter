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

#include "tnl.h"           // For Vector, types, and dSprintf
#include <string>
#include <sstream>
#include <iostream>
#include <sys/stat.h>      // For testing existence of folders

using namespace std;
using namespace TNL;

namespace Zap
{

// Collection of useful string things

string ExtractDirectory( const string& path )
{
  return path.substr( 0, path.find_last_of( '\\' ) + 1 );
}

string ExtractFilename( const string& path )
{
  return path.substr( path.find_last_of( '\\' ) + 1 );
}


string itos(S32 i)
{
   char outString[100];
   dSprintf(outString, sizeof(outString), "%d", i);
   return outString;
}


//// From http://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c
//bool caseInsensitiveStringCompare(const string &str1, const string &str2) {
//    if (str1.size() != str2.size()) {
//        return false;
//    }
//    for (string::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2) {
//        if (tolower(*c1) != tolower(*c2)) {
//            return false;
//        }
//    }
//    return true;
//}


// Convert string to lower case
string lcase(string strToConvert)
{
   for(U32 i = 0; i < strToConvert.length(); i++)
      strToConvert[i] = tolower(strToConvert[i]);
   return strToConvert;
}


// Convert string to upper case
string ucase(string strToConvert)
{
   for(U32 i = 0; i < strToConvert.length(); i++)
      strToConvert[i] = toupper(strToConvert[i]);
   return strToConvert;
}


// Ok, not strictly a string util, but do we really want a fileutils just for this??
bool fileExists(const string &path)
{
   struct stat st;
   return (stat(path.c_str(), &st) == 0);               // Does path exist?
}




// Based on http://www.gamedev.net/community/forums/topic.asp?topic_id=320087
// Parses a string on whitespace, except when inside "s
Vector<string> parseString(const string &line)
{
  Vector<string> result;

  string          item;
  stringstream    ss(line);

  while(ss >> item){
      if (item[0]=='"') {
      int lastItemPosition = item.length() - 1;
      if (item[lastItemPosition]!='"') {
        // read the rest of the double-quoted item
        string restOfItem;
        getline(ss, restOfItem, '"');
        item += restOfItem;
      }
      // otherwise, we had a single word that was quoted. In any case, we now
      // have the item in quotes; remove them
      item = item.substr(1, lastItemPosition-1);
    }
    // item is "fully cooked" now
    result.push_back(item);

  }
  return result;
}

/*
int main() {
  vector<string> result = parse("\"foo bar\" baz quux \"wibble\" \"1 2 3\"");
  for (int i = 0; i < result.size(); i++) {
    cout << "Item " << i+1 << ": " << result[i] << endl;
  }
}
*/

//Item 1: foo bar
//Item 2: baz
//Item 3: quux
//Item 4: wibble
//Item 5: 1 2 3



};

