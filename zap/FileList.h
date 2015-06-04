//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _FILELIST_H_
#define _FILELIST_H_

#include "tnlTypes.h"
#include "tnlVector.h"
#include <map>

using namespace std;
using namespace TNL;

namespace Zap 
{


typedef map<string, string>::iterator FileListIterator;

class FileList
{
private:
   bool mOk;
   map<string, string> mFileMap;
   void addFile(const string &dir, const string &filename);
   FileListIterator mIterator;

public:
   FileList();
   FileListIterator getIterator();
   string getCurrentFilename() const;              // Does not include path
   string getCurrentFullFilename() const;          // Includes path
   string getFullFilename(const string &filename); // Get full path for specified file

   bool isLast() const;
   void nextFile(bool wrap = false);
   void prevFile();
   void addFilesFromFolder(const string &dir);
   void addFilesFromFolder(const string &dir, const string extensions[], S32 extensionCount);
   void addFilesFromFolders(const Vector<string> &dirs, const string extensions[], S32 extensionCount);
   void removeFile(const string &file);
   bool isOk() const;
   S32 size() const;
};


}
#endif
