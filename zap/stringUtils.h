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

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#include "tnlVector.h"     // For Vector
#include "tnlTypes.h"

#include <string>
#include <map>

namespace Zap
{

using namespace std;
using namespace TNL;

// From http://stackoverflow.com/questions/134569/c-exception-throwing-stdstring
struct SaveException : public exception
{
   string msg;

   SaveException(string str);         // Constructor
   virtual ~SaveException() throw();  // Destructor, needed to avoid "looser throw specifier" errors with gcc
   const char* what() const throw();
};


// Collection of useful string things

string extractDirectory(const string &path);
string extractFilename(const string &path);
string extractExtension(const string &path);

string itos(S32 i);
string itos(U32 i);
string itos(U64 i);
string itos(S64 i);
string ftos(float f, int digits);
string ftos(float f);

S32 stoi(const string &s);
F64 stof(const string &s);

string replaceString(const string &strString, const string &strOld, const string &strNew);
string stripExtension(string filename);

string listToString(const Vector<string> &words, char seperator);

// TODO: Merge these methods
Vector<string> parseString(const string &line);
void parseString(const char *inputString, Vector<string> &words, char seperator);
void parseString(const string &inputString, Vector<string> &words, char seperator);
void parseComplexStringToMap(const string &inputString, map<string, string> &fillMap,
                             const string &entryDelimiter = ";", const string &keyValueDelimiter = ":");

string concatenate(const Vector<string> &words, S32 startingWith = 0);

string lcase(string strToConvert);
string ucase(string strToConvert);

bool isInteger(const char *str);

void s_fprintf(FILE *stream, const char *format, ...);      // throws SaveException

bool caseInsensitiveStringCompare(const string &str1, const string &str2);

// File utils
bool fileExists(const string &path);               // Does file exist?
bool makeSureFolderExists(const string &dir);      // Like the man said: Make sure folder exists
bool getFilesFromFolder(const string &dir, Vector<string> &files, const string extensions[] = 0, S32 extensionCount = 0);
bool safeFilename(const char *str);


// Different variations on joining file and folder names
string joindir(const string &path, const string &filename);
string strictjoindir(const string &part1, const string &part2);
string strictjoindir(const string &part1, const string &part2, const string &part3);

string trim_right(const string &source, const string &t = " ");
string trim_left(const string &source, const string &t = " ");
string trim(const string &source, const string &t = " ");

S32 countCharInString(const string &source, char search);

const U32 MAX_FILE_NAME_LEN = 128;     // Completely arbitrary
string makeFilenameFromString(const char *levelname);

string ctos(char c);

string writeLevelString(const char *in);

};

#endif
