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
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include "Settings.h"

#include "ConfigEnum.h"
#include "stringUtils.h"

using namespace TNL;

namespace Zap
{

// Convert a string value to a DisplayMode enum value
static DisplayMode stringToDisplayMode(string mode)
{
   if(lcase(mode) == "fullscreen-stretch")
      return DISPLAY_MODE_FULL_SCREEN_STRETCHED;
   else if(lcase(mode) == "fullscreen")
      return DISPLAY_MODE_FULL_SCREEN_UNSTRETCHED;
   else
      return DISPLAY_MODE_WINDOWED;
}


static YesNo stringToYesNo(string yesNo)
{
   return lcase(yesNo) == "yes" ? Yes : No;
}


static RelAbs stringToRelAbs(string relAbs)
{
   return lcase(relAbs) == "relative" ? Relative : Absolute;
}


template<> string      Setting<string>     ::fromString(const string &val) { return val; }
template<> S32         Setting<S32>        ::fromString(const string &val) { return atoi(val.c_str()); }
template<> DisplayMode Setting<DisplayMode>::fromString(const string &val) { return stringToDisplayMode(val); }
template<> YesNo       Setting<YesNo>      ::fromString(const string &val) { return stringToYesNo(val); }
template<> RelAbs      Setting<RelAbs>     ::fromString(const string &val) { return stringToRelAbs(val); }


////////////////////////////////////////
////////////////////////////////////////

// Constructor
AbstractSetting::AbstractSetting(const string &name, const string &key, const string &section, const string &comment):
   mName(name), mIniKey(key), mIniSection(section), mComment(comment)
{
   // Do nothing
}


AbstractSetting::~AbstractSetting()
{
   // Do nothing
}


string AbstractSetting::getName()    const { return mName; }
string AbstractSetting::getKey()     const { return mIniKey; }
string AbstractSetting::getSection() const { return mIniSection; }
string AbstractSetting::getComment() const { return mComment; }


////////////////////////////////////////
////////////////////////////////////////


// Destructor
Settings::~Settings()
{
   mSettings.deleteAndClear();
}


AbstractSetting *Settings::getSetting(const string &name)
{
   TNLAssert(mKeyLookup.find(name) != mKeyLookup.end(), "Setting with specified name not found!");

   return mSettings[mKeyLookup.find(name)->second];
}


void Settings::add(AbstractSetting *setting)
{
   mSettings.push_back(setting);
   mKeyLookup[setting->getName()] = mSettings.size() - 1;
}


string Settings::getStrVal(const string &name) const
{
   return mSettings[mKeyLookup.find(name)->second]->getValueString();
}


string Settings::getDefaultStrVal(const string &name) const
{
   return mSettings[mKeyLookup.find(name)->second]->getDefaultValueString();
}


string Settings::getKey(const string &name) const
{
   return mSettings[mKeyLookup.find(name)->second]->getKey();
}


string Settings::getSection(const string &name) const
{
   return mSettings[mKeyLookup.find(name)->second]->getSection();
}


// There are much more efficient ways to do this!
Vector<AbstractSetting *> Settings::getSettingsInSection(const string &section) const
{
   Vector<AbstractSetting *> settings;
   for(S32 i = 0; i < mSettings.size(); i++)
   {
      if(mSettings[i]->getSection() == section)
         settings.push_back(mSettings[i]);
   }

   return settings;
}


////////////////////////////////////////
////////////////////////////////////////


template <class T>
Setting<T>::Setting(const string &name, const T &defaultValue, const string &iniKey, const string &iniSection, const string &comment):
   Parent(name, iniKey, iniSection, comment),
   mDefaultValue(defaultValue),
   mValue(defaultValue)
{
   // Do nothing
}


template <class T>
Setting<T>::~Setting()
{
   // Do nothing
}


template <class T>
T Setting<T>::getValue() const
{
   return mValue;
}


template <class T>
void Setting<T>::setValue(const T &value)
{
   mValue = value;
}


template <class T>
string Setting<T>::getValueString() const
{
   return toString(mValue);
}


template <class T>
string Setting<T>::getDefaultValueString() const
{
   return toString(mDefaultValue);
}


template <class T>
void Setting<T>::setValFromString(const string &value)
{
   setValue(fromString(value));
}


////////////////////////////////////////
////////////////////////////////////////

// In order to keep the template definitions in the cpp file, we need to declare which template
// parameters we will use:
template class Setting<string>;
template class Setting<S32>;
template class Setting<DisplayMode>;
template class Setting<YesNo>;
template class Setting<RelAbs>;


}