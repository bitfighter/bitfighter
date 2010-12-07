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
CIniFile::CIniFile( string const iniPath)
{
  Path(iniPath);
  caseInsensitive = true;        // Case sensitivity creates confusion!
}


extern Zap::ZapJournal gZapJournal;
extern CIniFile gINI;

void CIniFile::ReadFile()
{
   // Normally you would use ifstream, but the SGI CC compiler has
   // a few bugs with ifstream. So ... fstream used.
   fstream f;
   string line;

   Vector<TNL::StringPtr> iniLines;
   if(gZapJournal.getCurrentMode() != TNL::Journal::Playback)
   {
      string x = path;
      string y(x.begin(), x.end());
      y.assign(x.begin(), x.end());

      f.open( y.c_str(), ios::in);

      // To better handle our journaling requirements, we'll read the INI file,
      // then pass those off to a journaled routine for processing.
      if (!f.fail())    // This is true if the file cannot be opened or something... in which case we don't want to read the file!
      {
         while(getline( f, line))
            if(line.length() > 2)                   // Anything shorter can't be useful...  strip these out here to reduce journaling burden
               iniLines.push_back(line.c_str());
         f.close();
      }
   }

   gZapJournal.setINILength(iniLines.size());         // This will get our INI vector length into the journal
   gZapJournal.processNextJournalEntry();             // And this will retrieve the value during playback

   for(S32 i = 0; i < gINI.lineCount; i++)
      if(gZapJournal.getCurrentMode() != TNL::Journal::Playback)
         gZapJournal.processINILine(iniLines[i]);     // Process our INI lines, will provide sensible defaults for any missing or malformed entries
      else
         gZapJournal.processNextJournalEntry();       // If we are playing back from a journal, this should cause the journaled INI settings to be processed

}

TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, setINILength, (S32 lineCount), (lineCount))
{
   gINI.lineCount = lineCount;
}


TNL_IMPLEMENT_JOURNAL_ENTRYPOINT(ZapJournal, processINILine, (TNL::StringPtr iniLine), (iniLine))
{
   gINI.processLine(iniLine.getString());
}

// Parse a single line of the INI file and break it into its component bits
void CIniFile::processLine(string line)
{
   string vn, v;
   string valuename, value;
   string::size_type pLeft, pRight;

   //  line = _TCHAR(l.c_str());
   // To be compatible with Win32, check for existence of '\r'.
   // Win32 files have the '\r' at the end of a line, and Unix files don't.
   // Note that the '\r' will be written to INI files from
   // Unix so that the created INI file can be read under Win32
   // without change.
   if ( (line.length() > 0) && line[line.length() - 1] == '\r')
   line = line.substr( 0, line.length() - 1);

   if ( line.length()) {
      if (( pLeft = line.find_first_of(";#[=")) != string::npos) {
         switch ( line[pLeft]) {
            case '[':
               if ((pRight = line.find_last_of("]")) != string::npos && pRight > pLeft) {
                  keyname = line.substr( pLeft + 1, pRight - pLeft - 1);
                  AddKeyName( keyname);
               }
               break;

            case '=':
               valuename = line.substr( 0, pLeft);
               value = line.substr( pLeft + 1);
               vn = valuename.c_str();
               v = value.c_str();
               vn = trim(vn, " ");
               v = trim(v, " ");

               SetValue( keyname, vn, v);
               break;

            case ';':
            case '#':
               if ( !names.size())
                  HeaderComment( line.substr( pLeft + 1));
               else
                  KeyComment( keyname, line.substr( pLeft + 1));
               break;
         }
      }
   }
}

bool CIniFile::WriteFile()
{
   // We don't want to write anything during journal playback, so if that's what's happening, we'll bail now
   if(gZapJournal.getCurrentMode() == TNL::Journal::Playback)
      return true;


  unsigned commentID, keyID, valueID;

  // Normally you would use ofstream, but the SGI CC compiler has a few bugs with ofstream. So ... fstream used
  fstream f;
  string x = path;
  string y(x.begin(), x.end());
  y.assign(x.begin(), x.end());
  f.open( y.c_str(), ios::out);

  if ( f.fail())
    return false;

  // Write header comments.
  for ( commentID = 0; commentID < comments.size(); ++commentID)
    f << ";" << comments[commentID] << iniEOL;
  if ( comments.size())
    f << iniEOL;

  // Write keys and values.
  for ( keyID = 0; keyID < keys.size(); ++keyID) {
    f << '[' << names[keyID] << ']' << iniEOL;
    // Comments.
    for ( commentID = 0; commentID < keys[keyID].comments.size(); ++commentID)
      f << ";" << keys[keyID].comments[commentID] << iniEOL;
    // Values.
    for ( valueID = 0; valueID < keys[keyID].names.size(); ++valueID)
      f << keys[keyID].names[valueID] << '=' << keys[keyID].values[valueID] << iniEOL;
    f << iniEOL;
  }
  f.close();

  return true;
}

long CIniFile::FindKey( string const keyname) const
{
  for ( unsigned keyID = 0; keyID < names.size(); ++keyID)
    if ( CheckCase( names[keyID]) == CheckCase( keyname))
      return long(keyID);
  return noID;
}

long CIniFile::FindValue( unsigned const keyID, string const valuename) const
{
  if ( !keys.size() || keyID >= keys.size())
    return noID;

  for ( unsigned valueID = 0; valueID < keys[keyID].names.size(); ++valueID)
    if ( CheckCase( keys[keyID].names[valueID]) == CheckCase( valuename))
      return long(valueID);
  return noID;
}

unsigned CIniFile::AddKeyName( string const keyname)
{
  if(FindKey(keyname) != noID)            // Don't create duplicate keys!
    return noID;
  names.resize( names.size() + 1, keyname);
  keys.resize( keys.size() + 1);
  return (unsigned) names.size() - 1;
}

string CIniFile::KeyName( unsigned const keyID) const
{
  if ( keyID < names.size())
    return names[keyID];
  else
    return "";
}

unsigned CIniFile::NumValues( unsigned const keyID)
{
  if ( keyID < keys.size())
    return (unsigned) keys[keyID].names.size();
  return 0;
}

unsigned CIniFile::NumValues( string const &keyname)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return 0;
  return (unsigned) keys[keyID].names.size();
}

string CIniFile::ValueName( unsigned const keyID, unsigned const valueID) const
{
  if ( keyID < keys.size() && valueID < keys[keyID].names.size())
    return keys[keyID].names[valueID];
  return "";
}

string CIniFile::ValueName(string const keyname, unsigned const valueID) const
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return "";
  return ValueName( keyID, valueID);
}

bool CIniFile::SetValue( unsigned const keyID, unsigned const valueID, string const value)
{
  if ( keyID < keys.size() && valueID < keys[keyID].names.size())
    keys[keyID].values[valueID] = value;

  return false;
}

// Will create key if it does not exist if create is set to true
bool CIniFile::SetValue( string const keyname, string const valuename, string const value, bool const create)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID) {
    if ( create)
      keyID = long( AddKeyName( keyname));
    else
      return false;

    if (keyID == noID)
      keyID = FindKey(keyname);

    if (keyID == noID)     // Fish in a tree?  This should not be.
       return false;
  }

  long valueID = FindValue( unsigned(keyID), valuename);
  if ( valueID == noID) {
    if ( !create)
      return false;
    keys[keyID].names.resize( keys[keyID].names.size() + 1, valuename);
    keys[keyID].values.resize( keys[keyID].values.size() + 1, value);
  } else
    keys[keyID].values[valueID] = value;

  return true;
}


bool CIniFile::SetAllValues( const string &section, const string &prefix, const TNL::Vector<string> &values)
{
   char keyname[256];
   bool stat = true;
   for(S32 i = 0; i < values.size(); i++)
   {
      dSprintf(keyname, 255, "%s%d", prefix.c_str(), i);
      stat &= gINI.SetValue(section, keyname, values[i], true);
   }

   return stat;      // true if all worked, false if any failed
}


bool CIniFile::SetValueI( string const keyname, string const valuename, int const value, bool const create)
{
  char svalue[MAX_VALUEDATA];
  dSprintf(svalue, sizeof(svalue), "%d", value);
  string const x(svalue);

  return SetValue( keyname, valuename, x);
}

bool CIniFile::SetValueF( string const keyname, string const valuename, double const value, bool const create)
{
  char svalue[MAX_VALUEDATA];

  dSprintf( svalue, sizeof(svalue), "%f", value);
  return SetValue( keyname, valuename, string(svalue));
}

bool CIniFile::SetValueV( string const keyname, string const valuename, char *format, ...)
{
  va_list args;
  char value[MAX_VALUEDATA];

  va_start( args, format);
  dSprintf( value, sizeof(value), format, args);
  va_end( args);
  return SetValue( keyname, valuename, value);
}

string CIniFile::GetValue( unsigned const keyID, unsigned const valueID, string const defValue) const
{
  if ( keyID < keys.size() && valueID < keys[keyID].names.size())
    return keys[keyID].values[valueID];
  return defValue;
}

string CIniFile::GetValue( string const &keyname, string const &valuename, string const &defValue) const
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return defValue;

  long valueID = FindValue( unsigned(keyID), valuename);
  if ( valueID == noID)
    return defValue;

  return keys[keyID].values[valueID];
}


void CIniFile::GetAllValues(string const &section, TNL::Vector<string> &valueList)
{
   S32 numVals = gINI.NumValues(section);
   if(numVals > 0)
   {
      TNL::Vector<string> keys(numVals);

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


int CIniFile::GetValueI(string const keyname, string const valuename, int const defValue) const
{
  char svalue[MAX_VALUEDATA];

  dSprintf( svalue, sizeof(svalue), "%d", defValue);

  string val = GetValue(keyname, valuename, string(svalue));

  size_t len = val.size();
  string s;
  s.resize(len);
  for(size_t i = 0; i < len; i++)
      s[i] = static_cast<char>(val[i]);
  int i = atoi(s.c_str());

  return i;

}

double CIniFile::GetValueF(string const keyname, string const valuename, double const defValue) const
{
  char svalue[MAX_VALUEDATA];

  dSprintf( svalue, sizeof(svalue), "%f", defValue);

  string val = GetValue(keyname, valuename, string(svalue));
  size_t len = val.size();
  string s;
  s.resize(len);
  for(size_t i = 0; i < len; i++)
      s[i] = static_cast<char>(val[i]);

   return atof(s.c_str());
}

/*
// 16 variables may be a bit of over kill, but hey, it's only code.
unsigned CIniFile::GetValueV( string const keyname, string const valuename, char *format,
               void *v1, void *v2, void *v3, void *v4,
               void *v5, void *v6, void *v7, void *v8,
               void *v9, void *v10, void *v11, void *v12,
               void *v13, void *v14, void *v15, void *v16)
{
  string   value;
  // va_list  args;
  unsigned nVals;


  value = GetValue( keyname, valuename);
  if ( !value.length())
    return false;
  // Why is there not vsscanf() function. Linux man pages say that there is
  // but no compiler I've seen has it defined. Bummer!
  //
  // va_start( args, format);
  // nVals = vsscanf( value.c_str(), format, args);
  // va_end( args);

  nVals = sscanf( value.c_str(), format,
        v1, v2, v3, v4, v5, v6, v7, v8,
        v9, v10, v11, v12, v13, v14, v15, v16);

  return nVals;
}
*/

bool CIniFile::DeleteValue( string const keyname, string const valuename)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return false;

  long valueID = FindValue( unsigned(keyID), valuename);
  if ( valueID == noID)
    return false;

  // This looks strange, but is neccessary.
  vector<string>::iterator npos = keys[keyID].names.begin() + valueID;
  vector<string>::iterator vpos = keys[keyID].values.begin() + valueID;
  keys[keyID].names.erase( npos, npos + 1);
  keys[keyID].values.erase( vpos, vpos + 1);

  return true;
}

bool CIniFile::DeleteKey( string const keyname)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return false;

  // Now hopefully this destroys the vector lists within keys.
  // Looking at <vector> source, this should be the case using the destructor.
  // If not, I may have to do it explicitly. Memory leak check should tell.
  // memleak_test.cpp shows that the following not required.
  //keys[keyID].names.clear();
  //keys[keyID].values.clear();

  vector<string>::iterator npos = names.begin() + keyID;
  vector<key>::iterator    kpos = keys.begin() + keyID;
  names.erase( npos, npos + 1);
  keys.erase( kpos, kpos + 1);

  return true;
}

void CIniFile::Erase()
{
  // This loop not needed. The vector<> destructor seems to do
  // all the work itself. memleak_test.cpp shows this.
  //for ( unsigned i = 0; i < keys.size(); ++i) {
  //  keys[i].names.clear();
  //  keys[i].values.clear();
  //}
  names.clear();
  keys.clear();
  comments.clear();
}

void CIniFile::HeaderComment( string const comment)
{
  comments.resize( comments.size() + 1, comment);
}

string CIniFile::HeaderComment( unsigned const commentID) const
{
  if ( commentID < comments.size())
    return comments[commentID];
  return "";
}

bool CIniFile::DeleteHeaderComment( unsigned commentID)
{
  if ( commentID < comments.size()) {
    vector<string>::iterator cpos = comments.begin() + commentID;
    comments.erase( cpos, cpos + 1);
    return true;
  }
  return false;
}

unsigned CIniFile::NumKeyComments( unsigned const keyID) const
{
  if ( keyID < keys.size())
    return (unsigned) keys[keyID].comments.size();
  return 0;
}

unsigned CIniFile::NumKeyComments( string const keyname) const
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return 0;
  return (unsigned) keys[keyID].comments.size();
}

bool CIniFile::KeyComment( unsigned const keyID, string const comment)
{
  if ( keyID < keys.size()) {
    keys[keyID].comments.resize( keys[keyID].comments.size() + 1, comment);
    return true;
  }
  return false;
}

bool CIniFile::KeyComment( string const keyname, string const comment, bool const create)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID) {
    if ( create)
      keyID = long( AddKeyName( keyname));
    else
      return false;

    if (keyID == noID)
      keyID = FindKey(keyname);

    if (keyID == noID)     
       return false;
  }

  return KeyComment( unsigned(keyID), comment);
}


string CIniFile::KeyComment( unsigned const keyID, unsigned const commentID) const
{
  if ( keyID < keys.size() && commentID < keys[keyID].comments.size())
    return keys[keyID].comments[commentID];
  return "";
}

string CIniFile::KeyComment( string const keyname, unsigned const commentID) const
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return "";
  return KeyComment( unsigned(keyID), commentID);
}

bool CIniFile::DeleteKeyComment( unsigned const keyID, unsigned const commentID)
{
  if ( keyID < keys.size() && commentID < keys[keyID].comments.size()) {
    vector<string>::iterator cpos = keys[keyID].comments.begin() + commentID;
    keys[keyID].comments.erase( cpos, cpos + 1);
    return true;
  }
  return false;
}

bool CIniFile::DeleteKeyComment( string const keyname, unsigned const commentID)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return false;
  return DeleteKeyComment( unsigned(keyID), commentID);
}

bool CIniFile::DeleteKeyComments( unsigned const keyID)
{
  if ( keyID < keys.size()) {
    keys[keyID].comments.clear();
    return true;
  }
  return false;
}

bool CIniFile::DeleteKeyComments( string const keyname)
{
  long keyID = FindKey( keyname);
  if ( keyID == noID)
    return false;
  return DeleteKeyComments( unsigned(keyID));
}

string CIniFile::CheckCase( string s) const
{
  if ( caseInsensitive)
    for ( string::size_type i = 0; i < s.length(); ++i)
      s[i] = tolower(s[i]);
  return s;
}


};

