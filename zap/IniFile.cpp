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
#  define iniEOL endl
#else
#  define iniEOL '\r' << endl
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


// Destructor
CIniFile::~CIniFile()
{
   //WriteFile();  --> Crashes with VC++ 2008
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

   lineCount = iniLines.size();      // Set our INI vector length
   
   // Process our INI lines, will provide sensible defaults for any missing or malformed entries
   for(S32 i = 0; i < lineCount; i++)
      processLine(iniLines[i].getString());
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
               section = line.substr(pLeft + 1, pRight - pLeft - 1);
               addSection(section);
            }
            break;

         case '=':      // Key = Value pair
            key = line.substr(0, pLeft);
            value = line.substr(pLeft + 1);
            k = key.c_str();
            v = value.c_str();
            k = trim(k, " ");
            v = trim(v, " ");

            SetValue(section, k, v);
            break;

         case ';':      // Comments
         case '#':
            if(!sectionNames.size())
               headerComment(line.substr(pLeft + 1));
            else
               sectionComment(section, line.substr(pLeft + 1));
            break;
         }
      }
   }
}

bool CIniFile::WriteFile()
{
   S32 commentID, sectionId, keyID;

   // Normally you would use ofstream, but the SGI CC compiler has a few bugs with ofstream. So ... fstream used
   fstream f;
   string x = path;
   string y(x.begin(), x.end());
   y.assign(x.begin(), x.end());
   f.open(y.c_str(), ios::out);

   if(f.fail())
      return false;

   // Write header comments.
   for(commentID = 0; commentID < headerComments.size(); ++commentID)
      f << ";" << headerComments[commentID] << iniEOL;
   if(headerComments.size())
      f << iniEOL;

   // Write keys and values.
   for(sectionId = 0; sectionId < sections.size(); ++sectionId) {
      f << '[' << sectionNames[sectionId] << ']' << iniEOL;
      // Comments.
      for(commentID = 0; commentID < sections[sectionId].comments.size(); ++commentID)
         f << ";" << sections[sectionId].comments[commentID] << iniEOL;
      // Values.
      for(keyID = 0; keyID < sections[sectionId].keys.size(); ++keyID)
         f << sections[sectionId].keys[keyID] << '=' << sections[sectionId].values[keyID] << iniEOL;
      f << iniEOL;
   }
   f.close();

   return true;
}

S32 CIniFile::findSection(const string &sectionName) const
{
   for(S32 sectionId = 0; sectionId < sectionNames.size(); ++sectionId)
      if(CheckCase(sectionNames[sectionId], sectionName))
         return sectionId;
   return noID;
}

S32 CIniFile::FindValue(S32 const sectionId, const string &keyName) const
{
   if(!sections.size() || sectionId >= sections.size())
      return noID;

   for(S32 keyID = 0; keyID < sections[sectionId].keys.size(); ++keyID)
      if(CheckCase(sections[sectionId].keys[keyID], keyName))
         return keyID;
   return noID;
}

S32 CIniFile::addSection(const string keyname)
{
   if(findSection(keyname) != noID)            // Don't create duplicate keys!
      return noID;
   sectionNames.push_back(keyname);
   sections.resize(sections.size() + 1);
   return sectionNames.size() - 1;
}

string CIniFile::sectionName(S32 const sectionId) const
{
   if(sectionId < sectionNames.size())
      return sectionNames[sectionId];
   else
      return "";
}


string CIniFile::ValueName(S32 const sectionId, S32 const valueID) const
{
   if(sectionId < sections.size() && valueID < sections[sectionId].keys.size())
      return sections[sectionId].keys[valueID];
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
   if(sectionId < sections.size() && valueID < sections[sectionId].keys.size())
      sections[sectionId].values[valueID] = value;

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
      sections[sectionId].keys.push_back(valuename);
      sections[sectionId].values.push_back(value);
   } else
      sections[sectionId].values[valueID] = value;

   return true;
}


bool CIniFile::SetAllValues(const string &section, const string &prefix, const Vector<string> &values)
{
   char key[256];
   bool stat = true;
   for(S32 i = 0; i < values.size(); i++)
   {
      dSprintf(key, 255, "%s%d", prefix.c_str(), i);
      stat &= SetValue(section, key, values[i], true);
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


string CIniFile::GetValue(S32 const sectionId, S32 const keyID, const string &defValue) const
{
   if(sectionId < sections.size() && keyID < sections[sectionId].keys.size())
      return sections[sectionId].values[keyID];
   return defValue;
}


string CIniFile::GetValue(S32 const sectionId, const string &keyName, const string &defValue) const
{
   S32 valueID = FindValue(sectionId, keyName);
   if(valueID == noID)
      return defValue;

   return sections[sectionId].values[valueID];
}


string CIniFile::GetValue(const string &section, const string &keyName, const string &defValue) const
{
   S32 sectionId = findSection(section);
   if(sectionId == noID)
      return defValue;

   S32 valueID = FindValue(sectionId, keyName);
   if(valueID == noID)
      return defValue;

   return sections[sectionId].values[valueID];
}


// Fill valueList with values from all keys in the specified section.  Key names will be discarded.
void CIniFile::GetAllValues(S32 const sectionId, Vector<string> &valueList)
{
   for(S32 i = 0; i < sections[sectionId].keys.size(); i++)
      // Only return non-empty values
      if(sections[sectionId].values[i] != "")
         valueList.push_back(sections[sectionId].values[i]);
}


// Fill valueList with values from all keys in the specified section.  Key names will be discarded.
void CIniFile::GetAllValues(const string &section, Vector<string> &valueList)
{
   S32 sectionId = findSection(section);
   if(sectionId == noID)
      return;

   GetAllValues(sectionId, valueList);
}


void CIniFile::GetAllKeys(S32 const sectionId, Vector<string> &keyList)
{
   if(GetNumEntries(sectionId) == 0)
      return;

   for(S32 i = 0; i < sections[sectionId].keys.size(); i++)
      keyList.push_back(sections[sectionId].keys[i]);
}


void CIniFile::GetAllKeys(const string &section, Vector<string> &keyList)
{
   S32 sectionId = findSection(section);
   if(sectionId == noID)
      return;

   GetAllKeys(sectionId, keyList);
}


int CIniFile::GetValueI(const string &section, const string &key, S32 const defValue) const
{
   char svalue[MAX_VALUEDATA];

   dSprintf(svalue, sizeof(svalue), "%d", defValue);

   string val = GetValue(section, key, string(svalue));

   std::size_t len = val.size();
   string s;
   s.resize(len);
   for(std::size_t i = 0; i < len; i++)
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
   std::size_t len = val.size();
   string s;
   s.resize(len);
   for(std::size_t i = 0; i < len; i++)
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
   sections[sectionId].keys.erase(valueID);
   sections[sectionId].values.erase(valueID);

   return true;
}


bool CIniFile::deleteSection(const string &section)
{
   S32 sectionId = findSection(section);
   if(sectionId == noID)
      return false;

   sectionNames.erase(sectionId);
   sections.erase(sectionId);

   return true;
}


void CIniFile::Erase()
{
   sectionNames.clear();
   sections.clear();
   headerComments.clear();
}

void CIniFile::headerComment(const string comment)
{
   headerComments.push_back(comment);
}


string CIniFile::headerComment(S32 const commentID) const
{
   if(commentID < headerComments.size())
      return headerComments[commentID];
   return "";
}


bool CIniFile::deleteHeaderComment(S32 commentID)
{
   if(commentID < headerComments.size()) {
      headerComments.erase(commentID);
      return true;
   }
   return false;
}


S32 CIniFile::numSectionComments(S32 const sectionId) const
{
   if(sectionId < sections.size())
      return sections[sectionId].comments.size();
   return 0;
}


S32 CIniFile::numSectionComments(const string keyname) const
{
   S32 sectionId = findSection(keyname);
   if(sectionId == noID)
      return 0;
   return sections[sectionId].comments.size();
}


bool CIniFile::sectionComment(S32 sectionId, const string &comment)
{
   if(sectionId < sections.size())
   {
      sections[sectionId].comments.push_back(comment);
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
   if(sectionId < sections.size() && commentID < sections[sectionId].comments.size())
      return sections[sectionId].comments[commentID];
   return "";
}


string CIniFile::sectionComment(const string keyname, S32 const commentID) const
{
   S32 sectionId = findSection(keyname);

   return sectionId == noID ? "" : sectionComment(sectionId, commentID);
}


bool CIniFile::deleteSectionComment(S32 const sectionId, S32 const commentID)
{
   if(sectionId < sections.size() && commentID < sections[sectionId].comments.size()) {
      sections[sectionId].comments.erase(commentID);
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
   if(sectionId < sections.size()) {
      sections[sectionId].comments.clear();
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
   for(S32 i = 0; i < sections.size() && result; i++)
      result &= deleteSectionComments(i);

   return result;
}


bool CIniFile::CheckCase(const string &s1, const string &s2) const
{
   if(caseInsensitive)
      return stricmp(s1.c_str(), s2.c_str()) == 0;
   else
      return s1 == s2;
}


void CIniFile::CaseSensitive()
{
   caseInsensitive = false;
}


void CIniFile::CaseInsensitive()
{
   caseInsensitive = true;
}


// Sets path of ini file to read and write from.
void CIniFile::Path(const string newPath)
{
   path = newPath;
}


string CIniFile::Path() const
{
   return path;
}


void CIniFile::SetPath(const string newPath)
{
   Path( newPath);
}


void CIniFile::Clear()
{
   Erase();
}


void CIniFile::Reset()
{
   Erase();
}


S32 CIniFile::NumSections() const
{
   return sectionNames.size();
}


S32 CIniFile::GetNumSections() const
{
   return NumSections();
}


string CIniFile::getSectionName( S32 const sectionId) const
{
   return sectionName(sectionId);
}


S32 CIniFile::GetNumEntries(S32 const sectionId)
{
   if(sectionId < sections.size())
      return sections[sectionId].keys.size();

   return 0;
}


S32 CIniFile::GetNumEntries(const string &keyName)
{
   S32 sectionId = findSection(keyName);

   if(sectionId == noID)
      return 0;

   return sections[sectionId].keys.size();
}


string CIniFile::GetValueName( S32 const sectionID, S32 const keyID) const
{
   return ValueName( sectionID, keyID);
}



string CIniFile::GetValueName( const string section, S32 const keyID) const
{
   return ValueName( section, keyID);
}


bool CIniFile::GetValueB(const string &section, const string &key, bool const defValue) const
{
   return (GetValueI( section, key, int( defValue)) != 0);
}


bool CIniFile::SetValueB(const string &section, const string &key, bool const value, bool const create)
{
   return SetValueI(section, key, int(value), create);
}


bool CIniFile::setValueYN(const string section, const string key, bool const value, bool const create)
{
   return SetValue(section, key, value ? "Yes" : "No", create);
}


std::size_t CIniFile::NumHeaderComments()
{
   return headerComments.size();
}


void CIniFile::deleteHeaderComments()
{
   headerComments.clear();
}


};

