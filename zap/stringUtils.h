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

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "tnlVector.h"     // For Vector
#include "tnlTypes.h"
#include <string>

using namespace std;

namespace Zap
{

// Collection of useful string things

string ExtractDirectory(const std::string& path);

string ExtractFilename(const std::string& path);

string itos(int i);
string ftos(float f, int digits);

// TODO: Merge these methods
TNL::Vector<string> parseString(const string &line);
void parseString(const char *inputString, TNL::Vector<string> &words, char seperator);

string concatenate(const TNL::Vector<string> &words, TNL::S32 startingWith = 0);

string lcase(string strToConvert);
string ucase(string strToConvert);



//bool caseInsensitiveStringCompare(const string &str1, const string &str2);

// File utils
bool fileExists(const string &path);         // Does file exist?

// Two different variations on joining file and folder names
string strictjoindir(const string &part1, const string &part2);
string strictjoindir(const string &part1, const string &part2, const string &part3);

string trim_right(const string &source, const string &t = " ");
string trim_left(const string &source, const string &t = " ");
string trim(const string &source, const string &t = " ");

};

#endif
