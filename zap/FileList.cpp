//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "FileList.h"

#include "stringUtils.h"

#include <algorithm>       // For std::random_shuffle
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
   currentItemIndex = -1;
}


bool FileList::isOk() const
{
   return mOk;
}


bool FileList::isLast() const
{
   return currentItemIndex == mFileMap.size() - 1;
}


S32 FileList::size() const
{
   return mFileMap.size();
}


// Get filename (no path)
string FileList::getCurrentFilename() const
{
   return mFileMap.find(mAccessList[currentItemIndex])->first;
}


// Get filename with path
string FileList::getCurrentFullFilename() const
{
   return mFileMap.find(mAccessList[currentItemIndex])->second;
}


// Get filename with path give filename w/o path
string FileList::getFullFilename(const string &filename)
{
   FileListIterator iterator = mFileMap.find(filename);

   if(iterator == mFileMap.end())
      return "";

   return iterator->second;
}


// Advance iterator to next file, wrap around to the beginning if we've hit the end
void FileList::nextFile(bool wrap)
{
   if(wrap && isLast())
      currentItemIndex = 0;
   else
      currentItemIndex++;
}


// Move iterator to previous file, wrap around to the end if we've hit the beginning
void FileList::prevFile()
{
   if(currentItemIndex == 0)
      currentItemIndex = mFileMap.size() - 1;
   else
      currentItemIndex--;
}


void FileList::removeFile(const string &file)
{
   mFileMap.erase(file);
   S32 index = mAccessList.getIndex(file);
   TNLAssert(index != -1, "Oops!");
   mAccessList.erase_fast(index);   // Does not preserve order... is this OK?  In all uses so far, it is...
   currentItemIndex = 0;            // Reset index
}


bool FileList::hasFile(const string &file) const
{
   return mFileMap.find(file) != mFileMap.end();
}


// Private, internal method
void FileList::addFile(const string &dir, const string &filename)
{
   // Only add the element if it does not yet exist... i.e. do not clobber existing, earlier values
   if(mFileMap.find(filename) == mFileMap.end())
   {
      mFileMap[filename] = strictjoindir(dir, filename);
      mAccessList.push_back(filename);
   }
}


void FileList::addFilesFromFolder(const string &dir)
{
   addFilesFromFolder(dir, NULL, 0);

   currentItemIndex = 0;    // Reset index
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

   currentItemIndex = 0;    // Reset index
}


// Randomize the order by which files will be accessed
void FileList::shuffle()
{
   std::random_shuffle(mAccessList.getStlVector().begin(), mAccessList.getStlVector().end());
}


}
