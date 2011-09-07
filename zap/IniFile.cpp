// IniFile.cpp:  Implementation of the CIniFile class.
// Written by:   Adam Clauss
// Email: cabadam@houston.rr.com
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
//
// Rewritten by: Shane Hill
// Date:         21/08/2001
// Email:        Shane.Hill@dsto.defence.gov.au
// Reason:       Remove dependancy on MFC. Code should compile on any
//               platform.
//
// Modified by Chris Eykamp to incorporate journaling for Zap
// Also replaced sprintf with dSprintf for more secure operations
// Also added a check to ensure duplicate keys aren't created
////////////////////////

//#define UNICODE
//#define _UNICODE

//#include <tchar.h>

//#include "../tnl/tnlLog.h"        // For logprintf

// C++ Includes
//#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>

#include "tnlJournal.h"       // For journaling support
#include "stringUtils.h"      // for lcase
#include "zapjournal.h"
#include "tnlTypes.h"


// C Includes
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

// Local Includes
#include "IniFile.h"

//#include <windows.h>    // For wsprintf


#if defined(WIN32)
#define iniEOL endl
#else
#define iniEOL '\r' << endl
#endif

using namespace std;

namespace Zap
{


// Constructor
CIniFile::CIniFile(const string iniPath)
{
   Path(iniPath);
   caseInsensitive = true;        // Case sensitivity creates confusion!
}


extern Zap::ZapJournal gZapJournal;

void CIniFile::ReadFile()
{
   // Normally you would use ifstream, but the SGI CC compiler has
   // a few bugs with ifstream. So ... fstream used.
   fstream f;
   string line;

   Vector<StringPtr> iniLines;
   if(gZapJournal.getCurrentMode() != TNL::Journal::Playback)
   {
      string x = path;
      string y(x.begin(), x.end());
      y.assign(x.begin(), x.end());

      f.open(y.c_str(), ios::in);

      // To better handle our journaling requirements, we'll read the INI file,
      // then pass those off to a journaled routine for processing.
      if(!f.fail())    // This is true if the file cannot be opened or something... in which case we don't want to read the file!
      {
         while(getline(f, line))
            if(line.length() > 2)                   // Anything shorter can't be useful...  strip these out here to reduce journaling burden
               iniLines.push_back(line.c_str());
         f.close();
      }
   }

   gZapJournal.setINILength(iniLines.size());      // This will get our INI vector length into the journal
   gZapJournal.processNextJournalEntry();          // And this will retrieve the value during playback
   
   for(S32 i = 0; i < gINI.lineCount; i++)
      gZapJournal.processINILine(iniLines[i]);     // Process our INI lines, will provide sensible defaults for any missing or malformed entries

}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, setINILength, (S32 lineCount), (lineCount))
{
   gINI.lineCount = lineCount;
}


TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, processINILine, (StringPtr iniLine), (iniLine))
{
   gINI.processLine(iniLine.getString());
}

// Parse a single line of the INI file and break it into its component bits
void CIniFile::processLine(string line)
{
   string k, v;
   string key, value;
   string::size_type pLeft, pRight;

   //  line = _TCHAR(l.c_str());
   // To be compatible with Win32, check for existence of '\r'.
   // Win32 files have the '\r' at the end of a line, and Unix files don't.
   // Note that the '\r' will be written to INI files from
   // Unix so that the created INI file can be read under Win32
   // without change.
   if((line.length() > 0) && line[line.length() - 1] == '\r')
      line = line.substr(0, line.length() - 1);

   if(line.length()) {
      if((pLeft = line.find_first_of(";#[=")) != string::npos) {
         switch (line[pLeft]) {
         case '[':      // Section
            if((pRight = line.find_last_of("]")) != string::npos && pRight > pLeft) {
               keyname = line.substr(pLeft + 1, pRight - pLeft - 1);
               addSection(keyname);
            }
            break;

         case '=':      // Key = Value pair
            key = line.substr(0, pLeft);
            value = line.substr(pLeft + 1);
            k = key.c_str();
            v = value.c_str();
            k = trim(k, " ");
            v = trim(v, " ");

            SetValue(keyname, k, v);
            break;

         case ';':      // Comments
         case '#':
            if(!names.size())
               headerComment(line.substr(pLeft + 1));
            else
               sectionComment(keyname, line.substr(pLeft + 1));
            break;
         }
      }
   }
}

bool CIniFile::WriteFile()
{
   S32 commentID, sectionId, valueID;

   // Normally you would use ofstream, but the SGI CC compiler has a few bugs with ofstream. So ... fstream used
   fstream f;
   string x = path;
   string y(x.begin(), x.end());
   y.assign(x.begin(), x.end());
   f.open(y.c_str(), ios::out);

   if(f.fail())
      return false;

   // Write header comments.
   for(commentID = 0; commentID < comments.size(); ++commentID)
      f << ";" << comments[commentID] << iniEOL;
   if(comments.size())
      f << iniEOL;

   // Write keys and values.
   for(sectionId = 0; sectionId < keys.size(); ++sectionId) {
      f << '[' << names[sectionId] << ']' << iniEOL;
      // Comments.
      for(commentID = 0; commentID < keys[sectionId].comments.size(); ++commentID)
         f << ";" << keys[sectionId].comments[commentID] << iniEOL;
      // Values.
      for(valueID = 0; valueID < keys[sectionId].names.size(); ++valueID)
         f << keys[sectionId].names[valueID] << '=' << keys[sectionId].values[valueID] << iniEOL;
      f << iniEOL;
   }
   f.close();

   return true;
}

S32 CIniFile::findSection(const string keyname) const
{
   for(S32 sectionId = 0; sectionId < names.size(); ++sectionId)
      if(CheckCase(names[sectionId]) == CheckCase(keyname))
         return sectionId;
   return noID;
}

S32 CIniFile::FindValue(S32 const sectionId, const string valuename) const
{
   if(!keys.size() || sectionId >= keys.size())
      return noID;

   for(S32 valueID = 0; valueID < keys[sectionId].names.size(); ++valueID)
      if(CheckCase(keys[sectionId].names[valueID]) == CheckCase(valuename))
         return valueID;
   return noID;
}

S32 CIniFile::addSection(const string keyname)
{
   if(findSection(keyname) != noID)            // Don't create duplicate keys!
      return noID;
   names.push_back(keyname);
   keys.resize(keys.size() + 1);
   return names.size() - 1;
}

string CIniFile::sectionName(S32 const sectionId) const
{
   if(sectionId < names.size())
      return names[sectionId];
   else
      return "";
}

S32 CIniFile::NumValues(S32 const sectionId)
{
   if(sectionId < keys.size())
      return keys[sectionId].names.size();
   return 0;
}

S32 CIniFile::NumValues(const string &keyname)
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return 0;
   return keys[sectionId].names.size();
}

string CIniFile::ValueName(S32 const sectionId, S32 const valueID) const
{
   if(sectionId < keys.size() && valueID < keys[sectionId].names.size())
      return keys[sectionId].names[valueID];
   return "";
}

string CIniFile::ValueName(const string keyname, S32 const valueID) const
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return "";
   return ValueName(sectionId, valueID);
}

bool CIniFile::SetValue(S32 const sectionId, S32 const valueID, const string value)
{
   if(sectionId < keys.size() && valueID < keys[sectionId].names.size())
      keys[sectionId].values[valueID] = value;

   return false;
}

// Will create key if it does not exist if create is set to true
bool CIniFile::SetValue(const string &keyname, const string &valuename, const string &value, bool const create)
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID) {
      if(create)
         sectionId = S64(addSection(keyname));
      else
         return false;

      if(sectionId == noID)
         sectionId = findSection(keyname);

      if(sectionId == noID)     // Fish in a tree?  This should not be.
         return false;
   }

   S32 valueID = FindValue(sectionId, valuename);
   if(valueID == noID) {
      if(!create)
         return false;
      keys[sectionId].names.push_back(valuename);
      keys[sectionId].values.push_back(value);
   } else
      keys[sectionId].values[valueID] = value;

   return true;
}


bool CIniFile::SetAllValues(const string &section, const string &prefix, const Vector<string> &values)
{
   char key[256];
   bool stat = true;
   for(S32 i = 0; i < values.size(); i++)
   {
      dSprintf(key, 255, "%s%d", prefix.c_str(), i);
      stat &= gINI.SetValue(section, key, values[i], true);
   }

   return stat;      // true if all worked, false if any failed
}


bool CIniFile::SetValueI(const string &section, const string &key, S32 const value, bool const create)
{
   char svalue[MAX_VALUEDATA];
   dSprintf(svalue, sizeof(svalue), "%d", value);
   const string x(svalue);

   return SetValue(section, key, x);
}


bool CIniFile::SetValueF(const string &section, const string &key, F64 const value, bool const create)
{
   char svalue[MAX_VALUEDATA];

   dSprintf(svalue, sizeof(svalue), "%f", value);
   return SetValue(section, key, string(svalue));
}

bool CIniFile::SetValueV(const string &section, const string &key, char *format, ...)
{
   va_list args;
   char value[MAX_VALUEDATA];

   va_start(args, format);
   dSprintf(value, sizeof(value), format, args);
   va_end(args);
   return SetValue(section, key, value);
}

string CIniFile::GetValue(S32 const sectionId, S32 const valueID, const string defValue) const
{
   if(sectionId < keys.size() && valueID < keys[sectionId].names.size())
      return keys[sectionId].values[valueID];
   return defValue;
}


string CIniFile::GetValue(const string &keyname, const string &valuename, const string &defValue) const
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return defValue;

   S32 valueID = FindValue(sectionId, valuename);
   if(valueID == noID)
      return defValue;

   return keys[sectionId].values[valueID];
}


void CIniFile::GetAllValues(const string &section, Vector<string> &valueList)
{
   S32 numVals = gINI.NumValues(section);
   if(numVals > 0)
   {
      Vector<string> keys(numVals);

      for(S32 i = 0; i < numVals; i++)
         keys.push_back(gINI.ValueName(section, i));

      string val;

      valueList.clear();

      for(S32 i = 0; i < numVals; i++)
      {
         val = gINI.GetValue(section, keys[i], "");
         if(val != "")
            valueList.push_back(val);
      }
   }
}


int CIniFile::GetValueI(const string &section, const string &key, S32 const defValue) const
{
   char svalue[MAX_VALUEDATA];

   dSprintf(svalue, sizeof(svalue), "%d", defValue);

   string val = GetValue(section, key, string(svalue));

   size_t len = val.size();
   string s;
   s.resize(len);
   for(size_t i = 0; i < len; i++)
      s[i] = static_cast<char>(val[i]);
   S32 i = atoi(s.c_str());

   return i;
}


// Returns true for "Yes" (case insensitive), false otherwise
bool CIniFile::GetValueYN(const string &section, const string &key, bool defValue) const
{
   return lcase(GetValue(section, key, defValue ? "Yes" : "No")) == "yes";
}


F64 CIniFile::GetValueF(const string &section, const string &key, F64 const defValue) const
{
   char svalue[MAX_VALUEDATA];

   dSprintf(svalue, sizeof(svalue), "%f", defValue);

   string val = GetValue(section, key, string(svalue));
   size_t len = val.size();
   string s;
   s.resize(len);
   for(size_t i = 0; i < len; i++)
      s[i] = static_cast<char>(val[i]);

   return atof(s.c_str());
}

/*
// 16 variables may be a bit of over kill, but hey, it's only code.
S32 CIniFile::GetValueV(const string keyname, const string valuename, char *format,
               void *v1, void *v2, void *v3, void *v4,
               void *v5, void *v6, void *v7, void *v8,
               void *v9, void *v10, void *v11, void *v12,
               void *v13, void *v14, void *v15, void *v16)
{
  string   value;
  // va_list  args;
  S32 nVals;


  value = GetValue(keyname, valuename);
  if(!value.length())
    return false;
  // Why is there not vsscanf() function. Linux man pages say that there is
  // but no compiler I've seen has it defined. Bummer!
  //
  // va_start(args, format);
  // nVals = vsscanf(value.c_str(), format, args);
  // va_end(args);

  nVals = sscanf(value.c_str(), format,
        v1, v2, v3, v4, v5, v6, v7, v8,
        v9, v10, v11, v12, v13, v14, v15, v16);

  return nVals;
}
 */

bool CIniFile::deleteKey(const string &section, const string &key)
{
   S32 sectionId = findSection(section);
   if(sectionId == noID)
      return false;

   S32 valueID = FindValue(sectionId, key);
   if(valueID == noID)
      return false;

   // This looks strange, but is neccessary.
   keys[sectionId].names.erase(valueID);
   keys[sectionId].values.erase(valueID);

   return true;
}


bool CIniFile::deleteSection(const string &section)
{
   S32 sectionId = findSection(section);
   if(sectionId == noID)
      return false;

   // Now hopefully this destroys the vector lists within keys.
   // Looking at <vector> source, this should be the case using the destructor.
   // If not, I may have to do it explicitly. Memory leak check should tell.
   // memleak_test.cpp shows that the following not required.
   //keys[sectionId].names.clear();
   //keys[sectionId].values.clear();

   names.erase(sectionId);
   keys.erase(sectionId);

   return true;
}


void CIniFile::Erase()
{
   // This loop not needed. The vector<> destructor seems to do
   // all the work itself. memleak_test.cpp shows this.
   //for(S32 i = 0; i < keys.size(); ++i) {
   //  keys[i].names.clear();
   //  keys[i].values.clear();
   //}
   names.clear();
   keys.clear();
   comments.clear();
}

void CIniFile::headerComment(const string comment)
{
   comments.push_back(comment);
}


string CIniFile::headerComment(S32 const commentID) const
{
   if(commentID < comments.size())
      return comments[commentID];
   return "";
}


bool CIniFile::deleteHeaderComment(S32 commentID)
{
   if(commentID < comments.size()) {
      comments.erase(commentID);
      return true;
   }
   return false;
}


S32 CIniFile::numSectionComments(S32 const sectionId) const
{
   if(sectionId < keys.size())
      return keys[sectionId].comments.size();
   return 0;
}


S32 CIniFile::numSectionComments(const string keyname) const
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return 0;
   return keys[sectionId].comments.size();
}


bool CIniFile::sectionComment(S32 sectionId, const string &comment)
{
   if(sectionId < keys.size())
   {
      keys[sectionId].comments.push_back(comment);
      return true;
   }

   // Invalid sectionId, not adding comment
   return false;
}


bool CIniFile::sectionComment(const string section, const string comment, bool const create)
{
   S32 sectionId = findSection(section);

   if(sectionId == noID) 
   {
      if(create)
         sectionId = S64(addSection(section));
      else
         return false;

      // I think this should never happen -CE 1/8/2011
      // If addsection only returns noID if section exists
      // if section exists, then it would have been found at top of function

      //if(sectionId == noID)
      //   sectionId = findSection(section);

      if(sectionId == noID)     
         return false;
   }

   return sectionComment(sectionId, comment);
}


string CIniFile::sectionComment(S32 const sectionId, S32 const commentID) const
{
   if(sectionId < keys.size() && commentID < keys[sectionId].comments.size())
      return keys[sectionId].comments[commentID];
   return "";
}


string CIniFile::sectionComment(const string keyname, S32 const commentID) const
{
   S32 sectionId = findSection(keyname);

   return sectionId == noID ? "" : sectionComment(sectionId, commentID);
}


bool CIniFile::deleteSectionComment(S32 const sectionId, S32 const commentID)
{
   if(sectionId < keys.size() && commentID < keys[sectionId].comments.size()) {
      keys[sectionId].comments.erase(commentID);
      return true;
   }
   return false;
}


bool CIniFile::deleteSectionComment(const string keyname, S32 const commentID)
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return false;
   return deleteSectionComment(sectionId, commentID);
}


bool CIniFile::deleteSectionComments(S32 const sectionId)
{
   if(sectionId < keys.size()) {
      keys[sectionId].comments.clear();
      return true;
   }
   return false;
}


bool CIniFile::deleteSectionComments(const string keyname)
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return false;
   return deleteSectionComments(sectionId);
}


bool CIniFile::deleteAllSectionComments()
{
   bool result = true;
   for(S32 i = 0; i < keys.size() && result; i++)
      result &= deleteSectionComments(i);

   return result;
}


string CIniFile::CheckCase(string s) const
{
   if(caseInsensitive)
      for(string::size_type i = 0; i < s.length(); i++)
         s[i] = tolower(s[i]);
   return s;
}


};

