//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _TNL_STRING_H_
#define _TNL_STRING_H_

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

namespace TNL
{

struct StringData
{
   U32 mRefCount;
   char mStringData[1];
};

class StringPtr
{
   StringData *mString;
   void alloc(const char *string)
   {
      mString = (StringData *) malloc(sizeof(StringData) + strlen(string));
      strcpy(mString->mStringData, string);
      mString->mRefCount = 1;
   }
   void decRef()
   {
      if(mString && !--mString->mRefCount)
         free(mString);
   }
public:
   StringPtr()
   {
      mString = NULL;
   }
   StringPtr(const char *string)
   {
      alloc(string);
   }
   StringPtr(const StringPtr &string)
   {
      mString = string.mString;
      if(mString)
         mString->mRefCount++;
   }
   StringPtr(const std::string &string)
   {
      alloc(string.c_str());
   }
   ~StringPtr()
   {
      decRef();
   }
   StringPtr &operator=(const StringPtr &ref)
   {
      decRef();
      mString = ref.mString;
      mString->mRefCount++;
      return *this;
   }
   StringPtr &operator=(const char *string)
   {
      decRef();
      alloc(string);
      return *this;
   }
   operator const char *() const
   {
      if(mString)
         return mString->mStringData;
      else
         return "";
   }
   const char *getString() const
   {
      if(mString)
         return mString->mStringData;
      else
         return "";
   }
};

};

#endif
