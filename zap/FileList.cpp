//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FileList.h"

#include "stringUtils.h"

#include <sys/stat.h>      // For testing existence of folders

#ifdef TNL_OS_WIN32
#  include "../other/dirent.h"   // Need local copy for Windows builds
#else
#  include <dirent.h>            // Need standard copy for *NIXes
#endif
namespace Zap
{


// Constructor
FileList::FileList()
{
   mOk = false;
}


bool FileList::isOk() const
{
   return mOk;
}


bool FileList::isLast() const
{
   return mIterator == mFileMap.end();
}


S32 FileList::size() const
{
   return mFileMap.size();
}


FileListIterator FileList::getIterator()
{
   return mFileMap.begin();
   //for(it_type iterator = m.begin(); iterator != m.end(); iterator++) {
}


string FileList::getCurrentFullFilename() const
{
   return mIterator->second;
}


string FileList::getFullFilename(const string &filename)
{
   FileListIterator iterator = mFileMap.find(filename);

   if(iterator == mFileMap.end())
      return "";

   return iterator->second;
}


string FileList::getCurrentFilename() const
{
   return mIterator->first;
}


// Advance iterator to next file, wrap around to the beginning if we've hit the end
void FileList::nextFile(bool wrap)
{
   mIterator++;

   if(wrap && isLast())
      mIterator = mFileMap.begin();
}


// Move iterator to previous file, wrap around to the end if we've hit the beginning
void FileList::prevFile()
{
   if(mIterator == mFileMap.begin())
      mIterator = mFileMap.end();

   mIterator--;
}


void FileList::removeFile(const string &file)
{
   mFileMap.erase(file);
}


bool FileList::hasFile(const string &file) const
{
   return mFileMap.find(file) != mFileMap.end();
}


void FileList::addFile(const string &dir, const string &filename)
{
   // Only add the element if it does not yet exist... i.e. do not clobber existing, earlier values
   if(mFileMap.find(filename) == mFileMap.end())
      mFileMap[filename] = strictjoindir(dir, filename);  
}


void FileList::addFilesFromFolder(const string &dir)
{
   addFilesFromFolder(dir, NULL, 0);

   mIterator = mFileMap.begin();    // Reset iterator
}


void FileList::addFilesFromFolders(const Vector<string> &dirs, const string extensions[], S32 extensionCount)
{
   for(S32 i = 0; i < dirs.size(); i++)
      addFilesFromFolder(dirs[i], extensions, extensionCount);
}


// Found files are not clobbered; i.e. the first one we find is the one we keep
void FileList::addFilesFromFolder(const string &dir, const string extensions[], S32 extensionCount)
{
   DIR *dp;
   struct dirent *dirp;

   if((dp = opendir(dir.c_str())) == NULL)
      return;

   while((dirp = readdir(dp)) != NULL)
   {
      string name = string(dirp->d_name);

      if(extensionCount > 0)
      {
         string extension = lcase(extractExtension(name));

         for(S32 i = 0; i < extensionCount; i++)      // Need to check if the extension is ok
         {
            if(name.length() > extensions[i].length() + 1)  // +1 -> include the dot '.'
            {
               string ext = lcase(extractExtension(extensions[i]));

               if(ext == extension)
                  addFile(dir, name);
            }
         }
      }
      else if(name != "." && name != "..")      // Don't include . and ..
         addFile(dir, name);
   }

   closedir(dp);

   mOk = true;
   mIterator = mFileMap.begin();    // Reset iterator5
}


}
