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

#include "stringUtils.h"
#include "tnlPlatform.h"   // For Vector, types, and dSprintf
#include "tnlVector.h"
#include "tnlLog.h"

#include <stdio.h>
#include <stdarg.h>        // For va_args
#include <streambuf>
#include <string>          // For... everything.  This is stringUtils, after all!
#include <sstream>         // For parseString
#include <sys/stat.h>      // For testing existence of folders

#include <boost/tokenizer.hpp>

#ifdef TNL_OS_WIN32
#  include <direct.h>        // For mkdir
#endif

#ifdef TNL_OS_WIN32
#  include "dirent.h"        // Need local copy for Windows builds
#else
#  include <dirent.h>        // Need standard copy for *NIXes
#endif


namespace Zap
{

using namespace boost;


// Constructor
SaveException::SaveException(string str) : msg(str)
{
   // Do nothing
}


// Destructor
SaveException::~SaveException() throw ()
{
   // Do nothing
}


const char *SaveException::what() const throw ()
{
   return msg.c_str();
}


////////////////////////////////////////
////////////////////////////////////////


// Collection of useful string things

string extractDirectory(const string &path )
{
   // Works on Windows and Linux/Mac!  (just don't have a path with a backslash on Linux/Mac)
  return path.substr( 0, path.find_last_of( "\\/" )); // Paths should never end with the slash
}

string extractFilename(const string &path )
{
  return path.substr( path.find_last_of( "\\/" ) + 1 );
}

string extractExtension(const string &path )
{
  return path.substr( path.find_last_of( '.' ) + 1 );
}


string itos(S32 i)
{
   char outString[12];  // 11 chars plus a null char, -2147483648
   dSprintf(outString, sizeof(outString), "%d", i);
   return outString;
}


string itos(U32 i)
{
   char outString[11];  // 10 chars plus a null char, 4294967295
   dSprintf(outString, sizeof(outString), "%u", i);
   return outString;
}


string itos(U64 i)
{
   char outString[21];  // 20 chars plus a null char, 18446744073709551615
   dSprintf(outString, sizeof(outString), "%u", i);
   return outString;
}


string itos(S64 i)
{
   char outString[21];  // 20 chars plus a null char, -9223372036854775808
   dSprintf(outString, sizeof(outString), "%d", i);
   return outString;
}


string stripZeros(string str)
{
   while(str[str.length() - 1]  == '0')
      str.erase(str.length() - 1);

   if(str[str.length() - 1] == '.')
      str.erase(str.length() - 1);

   return str;
}


string ftos(F32 f, S32 digits)
{
   char outString[100];
   dSprintf(outString, sizeof(outString), (string("%2.") + itos(digits) + "f").c_str(), f);

   return stripZeros(outString);
}


string ftos(F32 f)
{
   char outString[100];
   dSprintf(outString, sizeof(outString), "%f", f);

   return stripZeros(outString);
}

// These next two functions are defined in c++0x/c++11 and can be removed if we migrate to
// the new standard
S32 stoi(const string &s)
{
   return atoi(s.c_str());
}


F64 stof(const string &s)
{
   return atof(s.c_str());
}


// From http://stackoverflow.com/questions/3418231/c-replace-part-of-a-string-with-another-string, replaceAll variant
string replaceString(const string &strString, const string &from, const string &to) 
{
   string str = strString;    // Make working copy

   string::size_type start_pos = 0;
   while((start_pos = str.find(from, start_pos)) != string::npos)
   {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();      // In case 'to' contains 'from', like replacing 'x' with 'yx'
   }

   return str;
}


// Remove any extension from filename
string stripExtension(string filename)
{
   return filename.substr(0, filename.find_last_of('.'));
}


// From http://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c
bool caseInsensitiveStringCompare(const string &str1, const string &str2) {
    if(str1.size() != str2.size()) {
        return false;
    }
    for(string::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2) {
        if(tolower(*c1) != tolower(*c2)) {
            return false;
        }
    }
    return true;
}


// Convert string to lower case
string lcase(string strToConvert)      // Note that strToConvert is a copy of whatever was passed
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


// Return true if str looks like an int
bool isInteger(const char *str)
{
   S32 i = 0;
   while(str[i])
   {
      if(str[i] < '0' || str[i] > '9')
         return false;
      i++;
   }

   return true;
}


// TODO: Merge this with the one following
// Based on http://www.gamedev.net/community/forums/topic.asp?topic_id=320087
// Parses a string on whitespace, except when inside "s
//
// FIXME: This is using string streams which are an order of magnitude (or 2) too slow
// rewrite without using streams if possible
Vector<string> parseString(const string &line)
{
   Vector<string> result;

   string          item;
   stringstream    ss(line);

   while(ss >> item)
   {
      if(item[0] == '"')
      {
         S32 lastItemPosition = (S32)item.length() - 1;
         if(item[lastItemPosition] != '"') 
         {
            // read the rest of the double-quoted item
            string restOfItem;
            getline(ss, restOfItem, '"');
            item += restOfItem;
         }
         // otherwise, we had a single word that was quoted. In any case, we now
         // have the item in quotes; remove them
         item = trim(item, "\"");
      }

      // item is "fully cooked" now
      result.push_back(item);
   }

   return result;
}


void parseString(const string &inputString, Vector<string> &words, char seperator)
{
   parseString(inputString.c_str(), words, seperator);
}


// Splits inputString into a series of words using the specified separator; does not consider quotes
void parseString(const char *inputString, Vector<string> &words, char seperator)
{
   char word[128];
   S32 wn = 0;       // Where we are in the word we're creating
   S32 isn = 0;      // Where we are in the inputString we're parsing

   words.clear();

   while(inputString[isn] != 0)
   {
      if(inputString[isn] == seperator) 
      {
         word[wn] = 0;    // Add terminating NULL
         if(wn > 0) 
            words.push_back(word);
         wn = 0;
      }
      else
      {
         if(wn < 126)   // Avoid overflows
         {
            word[wn] = inputString[isn]; 
            wn++; 
         }
      }
      isn++;
   }
    word[wn] = 0;

    if(wn > 0) 
       words.push_back(word);
}


Vector<string> parseStringAndStripLeadingSlash(const char *str)
{
   Vector<string> words = parseString(str);

   if(words.size() > 0 && words[0][0] == '/')
      words[0].erase(0, 1);      // Remove leading /

   return words;
}


void parseComplexStringToMap(const string &inputString, map<string, string> &fillMap,
                             const string &entryDelimiter, const string &keyValueDelimiter)
{
   typedef tokenizer<char_separator<char> > tokenizer;

   char_separator<char> entrySeparator(entryDelimiter.c_str());
   char_separator<char> keyValueSeparator(keyValueDelimiter.c_str());

   // Tokenize the entries first
   tokenizer entries(inputString, entrySeparator);

   for (tokenizer::iterator iterator1 = entries.begin(); iterator1 != entries.end(); ++iterator1)
   {
      // Now tokenize the key and value
      tokenizer keyAndValues(*iterator1, keyValueSeparator);

      // Set iterator to second token.  Note that if there is no second token, the first is used again
      tokenizer::iterator iterator2 = keyAndValues.begin();
      iterator2++;

      // Add to map
      pair<string, string> keyValuePair(*keyAndValues.begin(),
            iterator2.current_token() == *keyAndValues.begin() ? "" : iterator2.current_token());
      fillMap.insert(keyValuePair);
   }
}


// Concatenate all strings in words into one, startingWith default to zero
// TODO: Merge with listToString below
string concatenate(const Vector<string> &words, S32 startingWith)
{
   string concatenated = "";
   for(S32 i = startingWith; i < words.size(); i++)
      concatenated += (i == startingWith ? "" : " ") + words[i];

   return concatenated;
}


// TODO: Merge with concatenate above
string listToString(const Vector<string> &words, char seperator)
{
   string str = "";

   // Convert separator char to a c_string so it can be added below
   char sep[2];
   sep[0] = seperator;
   sep[1] = 0;    // Null terminate
      
   for(S32 i = 0; i < words.size(); i++)
        str += words[i] + ((i < words.size() - 1) ? sep : "");

   return str;
}


// Safe fprintf ==> throws exception if writing fails
void s_fprintf(FILE *stream, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    if(fprintf(stream, "%s", buffer) < 0)     // Uh-oh...
    {
       throw(SaveException("Error writing to file"));
    }
}


string getFileSeparator()
{
#ifdef TNL_OS_WIN32
   return "\\";
#else
   return "/";
#endif
}

// Returns true if file or folder exists
// Ok, not strictly a string util, but do we really want a fileutils just for this??
bool fileExists(const string &path)
{
   struct stat st;
   return (stat(path.c_str(), &st) == 0);               // Does path exist?
}


// Checks if specified folder exists; creates it if not
bool makeSureFolderExists(const string &folder)
{
   if(!fileExists(folder))
   {
#ifdef TNL_OS_WIN32
      mkdir(folder.c_str());
#else
      mkdir(folder.c_str(), 0755);
#endif
      if(!fileExists(folder))
      {
         logprintf(LogConsumer::LogError, "Could not create folder %s", folder.c_str());
         return false;
      }
   }

   return true;
}


// Read files from folder
bool getFilesFromFolder(const string &dir, Vector<string> &files, const string extensions[], S32 extensionCount)
{
   DIR *dp;
   struct dirent *dirp;

   if((dp = opendir(dir.c_str())) == NULL)
      return false;

   while ((dirp = readdir(dp)) != NULL)
   {
      string name = string(dirp->d_name);

      if(extensionCount > 0) 
      {
         string extension = lcase(extractExtension(name));

         for(S32 i = 0; i < extensionCount; i++)
         {
            if(name.length() > extensions[i].length() + 1)  // +1 -> include the dot '.'
            {
               string ext = lcase(extractExtension(extensions[i]));
               if(ext == extension)
                  files.push_back(name);
            }
         }
      }
      else
         if (name.length() > 2)  // quick hack to not include . and ..
            files.push_back(name);
   }

   closedir(dp);
   return true;
}


// Make sure a file name is 'safe', i.e. not having a path component
bool safeFilename(const char *str)
{
   char chr = str[0];
   S32 i = 0;
   while(chr != 0)
   {
      if(chr == '\\' || chr == '/')
         return false;
      i++;
      chr = str[i];
   }
   return true;
}


bool copyFile(const string &sourceFilename, const string &destFilename)
{
   bool success = true;

   // Yay for C!
   FILE *sourceFile = fopen(sourceFilename.c_str(), "rb");  // Use binary 'b'
   FILE *destFile = fopen(destFilename.c_str(), "wb");      // This will overwrite!

   if(sourceFile != NULL && destFile != NULL)
   {
      U8 buffer[8192];  // XXX Big enough for efficiency?
      std::size_t readChunk;

      while((readChunk = fread(buffer, 1, sizeof buffer, sourceFile)) > 0)
      {
         std::size_t writtenChunk = fwrite(buffer, 1, readChunk, destFile);

         // Error writing file
         if(writtenChunk < 0)
            success = false;

         // Error writing file, Disk probably full
         else if(writtenChunk < readChunk)
            success = false;

         if(!success)
            break;
      }
   }
   else
      success = false;

   if(sourceFile)
      fclose(sourceFile);
   if(destFile)
      fclose(destFile);

   return success;
}


bool copyFileToDir(const string &sourceFilename, const string &destDir)
{
   string destFilename = destDir + getFileSeparator() + extractFilename(sourceFilename);

   return copyFile(sourceFilename, destFilename);
}


// Join a directory and filename strings in a platform-specific way
string joindir(const string &path, const string &filename)
{
   // If there is no path, there's nothing to join -- just return filename
   if(path == "")
      return filename;

   // Does path already have a trailing delimiter?  If so, we'll use that.
   if(path[path.length() - 1] == '\\' || path[path.length() - 1] == '/')
      return path + filename;

   // Otherwise, join with a delimeter.
   // Since mixed delimeters look like crap, we'll use whichever we find first to try to make them match.
   if(path.find('\\') != string::npos)
      return path + "\\" + filename;

   // If there are currently no delimeters in path, use good ol' trusty forward slash.
   return path + "/" + filename;
}
   

// Join without checking for blank parts
string strictjoindir(const string &part1, const string &part2)
{
   if(part1.length() == 0) return part2;      //avoid crash on zero length string.

   // Does path already have a trailing delimeter?  If so, we'll use that.
   if(part1[part1.length() - 1] == '\\' || part1[part1.length() - 1] == '/')
      return part1 + part2;

   // Otherwise, join with a delimeter.  This works on Win, OS X, and Linux.
   return part1 + "/" + part2;
}


// Three arg version
string strictjoindir(const string &part1, const string &part2, const string &part3)
{
   return strictjoindir(part1, strictjoindir(part2, part3));
}


string trim_right(const string &source, const string &t)
{
   string str = source;
   return str.erase(str.find_last_not_of(t) + 1);
}


string trim_left(const string &source, const string &t)
{
   string str = source;
   return str.erase(0, source.find_first_not_of(t));
}

string trim(const string &source, const string &t)
{
   return trim_left(trim_right(source, t), t);
}

// count the occurrence of a specific character in a string
S32 countCharInString(const string &source, char search)
{
    S32 count = 0, c = 0;
    while (source[c] != '\0')
    {
          if (source[c] == search)
              count++;
          c++;
    }
    return count;
}


string makeFilenameFromString(const char *levelname)
{
   static char filename[MAX_FILE_NAME_LEN + 1];    // Leave room for terminating null

   U32 i = 0;

   while(i < MAX_FILE_NAME_LEN && levelname[i] != 0)
   {
      // Prevent invalid characters in file names
      char c = levelname[i];
      if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
         filename[i]=c;
      else
         filename[i]='_';
      i++;
   }

   filename[i] = 0;    // Null terminate
   return filename;
}


string ctos(char c)
{
   // Return empty string if char is null
   if (!c)
      return "";

   return string(1, c);
}

string replaceString(const char *in, const char *find, const char *replace)
{
   string out;
   int n = 0;
   int findlen = strlen(find);
   while(in[n])
   {
      if(!strncmp(&in[n], find, findlen))
      {
         out += replace;
         n += findlen;
      }
      else
      {
         out += in[n];
         n++;
      }
   }
   return out;
}

string writeLevelString(const char *in)
{
   int c=0;
   while(in[c] != 0 && in[c] != '\"' && in[c] != '#' && in[c] != ' ')
      c++;
   if(in[c] == 0 && c != 0)
      return string(in);  // string does not need to be changed if not zero length and there is no space or any of: # "

   string out = replaceString(in, "\"", "\"\"");
   return string("\"") + out + "\"";
}

bool writeFile(const string& path, const string& contents, bool append)
{
   ofstream file(path.c_str(), ios::out);
   if(!file.is_open())
   {
      return false;
   }

   file << contents;
   if(!file.good())
   {
      file.close();
      return false;
   }

   file.close();
   return true;
}

const string readFile(const string& path)
{
   ifstream file(path.c_str());
   if(!file.is_open())
   {
      return "";
   }

   // The extra parentheses around the first argument are required. Removing
   // them will lead to the Most Vexing Parse problem
   // http://www.informit.com/guides/content.aspx?g=cplusplus&seqNum=439
   string result((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

   return result;
}

};

