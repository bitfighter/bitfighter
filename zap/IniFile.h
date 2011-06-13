// IniFile.cpp:  Implementation of the CIniFile class.
// Written by:   Adam Clauss
// Email: cabadam@tamu.edu
// You may use this class/code as you wish in your programs.  Feel free to distribute it, and
// email suggested changes to me.
//
// Rewritten by: Shane Hill
// Date:         21/08/2001
// Email:        Shane.Hill@dsto.defence.gov.au
// Reason:       Remove dependancy on MFC. Code should compile on any
//               platform. Tested on Windows/Linux/Irix
//////////////////////////////////////////////////////////////////////

#ifndef CIniFile_H
#define CIniFile_H

//#define _UNICODE

// C++ Includes
#include <string>

#include <tnlVector.h>     // For Vector type

// C Includes
#include <stdlib.h>

#define MAX_KEYNAME    128
#define MAX_VALUENAME  128
#define MAX_VALUEDATA 2048

using namespace std;
using namespace TNL;

namespace Zap
{

class CIniFile
{
private:
   bool   caseInsensitive;
   string path;
   struct key {
      Vector<string> names;
      Vector<string> values;
      Vector<string> comments;
   };
   Vector<key>    keys;     // <== should be sections
   Vector<string> names;    // <== should be keys
   Vector<string> comments;
   string CheckCase( string s) const;

   string keyname;    // <== should be section

public:
   enum errors{ noID = -1};
   S32 lineCount;

   CIniFile( const string iniPath = "");     // Constructor
   ~CIniFile()  {
      //WriteFile();  --> Crashes with VC++ 2008
   }

   void processLine(string line);     // Process a line of an input file (CE)


   // Sets whether or not keynames and valuenames should be case sensitive.
   // The default is case insensitive.
   void CaseSensitive()                           {caseInsensitive = false;}
   void CaseInsensitive()                         {caseInsensitive = true;}

   // Sets path of ini file to read and write from.
   void Path(const string newPath)                {path = newPath;}
   string Path() const                            {return path;}
   void SetPath(const string newPath)             {Path( newPath);}

   // Reads ini file specified using path.
   // Returns true if successful, false otherwise.
   void ReadFile();

   // Writes data stored in class to ini file.
   bool WriteFile();

   // Deletes all stored ini data.
   void Erase();
   void Clear()                                   {Erase();}
   void Reset()                                   {Erase();}

   // Returns index of specified key, or noID if not found.
   S32 findSection( const string section) const;

   // Returns index of specified value, in the specified key, or noID if not found.
   S32 FindValue( S32 const sectionID, const string key) const;

   // Returns number of keys currently in the ini.
   S32 NumKeys() const                        {return names.size();}
   S32 GetNumKeys() const                {return NumKeys();}

   // Add a key name.
   S32 addSection( const string section);

   // Returns key names by index.
   string sectionName( S32 const sectionId) const;
   string getSectionName( S32 const sectionId) const {return sectionName(sectionId);}

   // Returns number of values stored for specified key.
   S32 NumValues( S32 const sectionId);
   S32 GetNumValues( S32 const sectionId)   {return NumValues( sectionId);}
   S32 NumValues( const string &keyname);
   S32 GetNumValues( const string keyname)   {return NumValues( keyname);}

   // Returns value name by index for a given keyname or sectionId.
   string ValueName( S32 const sectionID, S32 const keyID) const;
   string GetValueName( S32 const sectionID, S32 const keyID) const {
      return ValueName( sectionID, keyID);
   }
   string ValueName( const string section, S32 const keyID) const;
   string GetValueName( const string section, S32 const keyID) const {
      return ValueName( section, keyID);
   }

   // Gets value of [keyname] valuename =.
   // Overloaded to return string, int, and double.
   // Returns defValue if key/value not found.
   string GetValue( S32 const sectionID, S32 const keyID, const string defValue = "") const;
   string GetValue(const string &section, const string &key, const string &defValue = "") const;

   // Load up valueList with all values from the section
   void GetAllValues(const string &section, TNL::Vector<string> &valueList);

   S32    GetValueI(const string &section, const string &key, S32 const defValue = 0) const;
   bool   GetValueB(const string &section, const string &key, bool const defValue = false) const {
      return (GetValueI( section, key, int( defValue)) != 0);
   }
   F64   GetValueF(const string &section, const string& key, F64 const defValue = 0.0) const;
   bool     GetValueYN(const string &section, const string &key, bool defValue) const;

   // This is a variable length formatted GetValue routine. All these voids
   // are required because there is no vsscanf() like there is a vsprintf().
   // Only a maximum of 8 variable can be read.
   /*  S32 GetValueV( const string keyname, const string valuename, char *format,
            void *v1 = 0, void *v2 = 0, void *v3 = 0, void *v4 = 0,
            void *v5 = 0, void *v6 = 0, void *v7 = 0, void *v8 = 0,
            void *v9 = 0, void *v10 = 0, void *v11 = 0, void *v12 = 0,
            void *v13 = 0, void *v14 = 0, void *v15 = 0, void *v16 = 0);
    */
   // Sets value of [keyname] valuename =.
   // Specify the optional paramter as false (0) if you do not want it to create
   // the key if it doesn't exist. Returns true if data entered, false otherwise.
   // Overloaded to accept string, int, and double.
   bool SetValue(const string &section, const string &key, const string &value, bool const create = true);
   bool SetAllValues(const string &section, const string &prefix, const TNL::Vector<string> &values);
   bool SetValueI(const string &section, const string &key, int const value, bool const create = true);
   bool SetValueB(const string &section, const string &key, bool const value, bool const create = true) {
      return SetValueI(section, key, int(value), create);
   }
   bool setValueYN(const string section, const string key, bool const value, bool const create = true) {
      return SetValue(section, key, value ? "Yes" : "No", create);
   }
   bool SetValueF(const string &section, const string &key, F64 const value, bool const create = true);
   bool SetValueV(const string &section, const string &key, char *format, ...);
   bool SetValue(S32 const sectionId, S32 const valueID, const string value);

   // Deletes specified value.
   // Returns true if value existed and deleted, false otherwise.
   bool deleteKey(const string &section, const string &key);

   // Deletes specified key and all values contained within.
   // Returns true if key existed and deleted, false otherwise.
   bool deleteSection(const string &section);

   // Header comment functions.
   // Header comments are those comments before the first key.
   //
   // Get number of header comments.
   size_t NumHeaderComments()   {return comments.size();}
   // Add a header comment.
   void     headerComment( const string comment);
   // Return a header comment.
   string   headerComment( S32 const commentID) const;
   // Delete a header comment.
   bool     deleteHeaderComment( S32 commentID);
   // Delete all header comments.
   void     deleteHeaderComments()               { comments.clear(); }

   // Key comment functions.
   // Key comments are those comments within a key. Any comments
   // defined within value names will be added to this list. Therefore,
   // these comments will be moved to the top of the key definition when
   // the CIniFile::WriteFile() is called.
   //
   // Number of key comments.
   S32 numSectionComments(S32 const sectionId) const;
   S32 numSectionComments(const string keyname) const;
   // Add a key comment.
   bool     sectionComment(S32 sectionId, const string &comment);
   bool     sectionComment(const string keyname, const string comment, bool const create = true);
   // Return a key comment.
   string   sectionComment( S32 const sectionId, S32 const commentID) const;
   string   sectionComment( const string keyname, S32 const commentID) const;
   // Delete a key comment.
   bool     deleteSectionComment( S32 const sectionId, S32 const commentID);
   bool     deleteSectionComment( const string keyname, S32 const commentID);
   // Delete all comments for a key.
   bool     deleteSectionComments( S32 const sectionId);
   bool     deleteSectionComments( const string keyname);

   bool     deleteAllSectionComments();
};

extern CIniFile gINI;

};
#endif

