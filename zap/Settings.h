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

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "tnlVector.h"

#include <string>
#include <map>

using namespace std;
using namespace TNL;

namespace Zap
{

class AbstractSetting
{
private:
   string mName;        // Value we use to look this item up
   string mIniKey;      // INI key
   string mIniSection;  // INI section
   string mComment;

public:
   AbstractSetting(const string &name, const string &key, const string &section, const string &comment);   // Constructor
   virtual ~AbstractSetting();                                                                             // Destructor

   string getName() const;
   string getKey() const;
   string getSection() const;

   virtual void setValFromString(const string &value) = 0;

   virtual string getValueString() const = 0;         // Returns current value, as a string
   virtual string getDefaultValueString() const = 0;  // Returns default value, as a string
   virtual string getComment() const;
};


////////////////////////////////////////
////////////////////////////////////////


template <class T>
class Setting : public AbstractSetting
{
   typedef AbstractSetting Parent;

private:
   T mDefaultValue;
   T mValue;

public:
   Setting<T>(const string &name, const T &defaultValue, const string &iniKey, const string &iniSection, const string &comment = "");  // Constructor
   virtual ~Setting<T>();                                                                                                              // Destructor

   T getValue() const;
   void setValue(const T &value);
   string getValueString() const;
   string getDefaultValueString() const;
   void setValFromString(const string &value);

   T fromString(const string &val);
};


////////////////////////////////////////
////////////////////////////////////////


// Container for all our settings
class Settings
{
private:
   map<string, S32> mKeyLookup;     // Maps string key to vector index; updated when item is added
   Vector<AbstractSetting *> mSettings;

public:
   ~Settings();      // Destructor

   AbstractSetting *getSetting(const string &name);
   void add(AbstractSetting *setting);

   template <class T>
   void setVal(const string &name, const T &value)
   {
      S32 key = mKeyLookup.find(name)->second;
      AbstractSetting *absSet = mSettings[key];
      TNLAssert(dynamic_cast<Setting<T> *>(absSet), "Expected setting!");

      static_cast<Setting<T> *>(absSet)->setValue(value);
   }


   template <class T>
   T getVal(const string &name) const
   {
      AbstractSetting *absSet = mSettings[mKeyLookup.find(name)->second];
      TNLAssert(dynamic_cast<Setting<T> *>(absSet), "Expected setting!");

      return static_cast<Setting<T> *>(absSet)->getValue();
   }

   string getStrVal(const string &name) const;
   string getDefaultStrVal(const string &name) const;
   string getKey(const string &name) const;
   string getSection(const string &name) const;

   Vector<AbstractSetting *> getSettingsInSection(const string &section) const;
};


};

#endif