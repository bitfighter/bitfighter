//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include "ConfigEnum.h"

#include "Color.h"
#include "stringUtils.h"

#include "tnlVector.h"

#include <string>
#include <map>

using namespace std;
using namespace TNL;

namespace Zap
{

template <class IndexType>
class AbstractSetting
{
private:
   IndexType mName;     // Value we use to look this item up
   string mIniKey;      // INI key
   string mIniSection;  // INI section
   string mComment;

public:
   // Constructor
   AbstractSetting(IndexType name, const string &key, const string &section, const string &comment):
      mIniKey(key), 
      mIniSection(section), 
      mComment(comment)
   {
      mName = name;
   }

   ~AbstractSetting() { /* Do nothing */ }      // Destructor

   IndexType getName()    const { return mName; }
   string    getKey()     const { return mIniKey; }
   string    getSection() const { return mIniSection; }
   string    getComment() const { return mComment; }

   virtual void setValFromString(const string &value) = 0;

   virtual string getValueString() const = 0;         // Returns current value, as a string
   virtual string getDefaultValueString() const = 0;  // Returns default value, as a string
};


////////////////////////////////////////
////////////////////////////////////////

// This class exists mainly to get past the C++ restriction about partial specialization
// of class methods.  By creating a new class, we can reduce our template parameters
// to one (DataType) from the two we have in Setting (DataType and IndexType).
class Evaluator
{
private:
   static DisplayMode stringToDisplayMode(string mode);
   static ColorEntryMode stringToColorEntryMode(string mode);
   static GoalZoneFlashStyle stringToGoalZoneFlashStyle(string style);
   static YesNo stringToYesNo(string yesNo);
   static RelAbs stringToRelAbs(string relAbs);

public:
   // Templated declaration
   template <class DataType>
   DataType fromString(const string &val) { TNLAssert(false, "Specialize me!"); }

   // Specializations
   template<> string             fromString(const string &val) { return val;                             }
   template<> S32                fromString(const string &val) { return atoi(val.c_str());               }
   template<> U32                fromString(const string &val) { return atoi(val.c_str());               }
   template<> U16                fromString(const string &val) { return atoi(val.c_str());               }
   template<> DisplayMode        fromString(const string &val) { return stringToDisplayMode(val);        }
   template<> YesNo              fromString(const string &val) { return stringToYesNo(val);              }
   template<> RelAbs             fromString(const string &val) { return stringToRelAbs(val);             }
   template<> ColorEntryMode     fromString(const string &val) { return stringToColorEntryMode(val);     }
   template<> GoalZoneFlashStyle fromString(const string &val) { return stringToGoalZoneFlashStyle(val); }
   template<> Color              fromString(const string &val) { return Color::iniValToColor(val);       }
};


////////////////////////////////////////
////////////////////////////////////////

template <class DataType, class IndexType>
class Setting : public AbstractSetting<IndexType>
{
   typedef AbstractSetting Parent;

private:
   DataType mDefaultValue;
   DataType mValue;
   Evaluator mEvaluator;

public:

   Setting(IndexType name, const DataType &defaultValue, const string &iniKey, const string &iniSection, const string &comment):
      Parent(name, iniKey, iniSection, comment),
      mDefaultValue(defaultValue),
      mValue(defaultValue)
   {
      // Do nothing
   }

   ~Setting() { /* Do nothing */ }

   DataType fromString(const string &val) { return mEvaluator.fromString<DataType>(val); }

   void setValue(const DataType &value)         { mValue = value;                 }

   DataType getValue() const                    { return mValue;                  }
   string getValueString() const                { return toString(mValue);        }
   string getDefaultValueString() const         { return toString(mDefaultValue); }
   
   void   setValFromString(const string &value) { setValue(fromString(value));    }
};


////////////////////////////////////////
////////////////////////////////////////


// Container for all our settings
template <class IndexType>
class Settings
{
private:
   map<IndexType, S32> mKeyLookup;     // Maps key to vector index; updated when item is added
   Vector<AbstractSetting<IndexType> *> mSettings;

public:
   Settings()  { /* Do nothing */ }             // Constructor
   ~Settings() { mSettings.deleteAndClear(); }  // Destructor


   template <class DataType>
   void setVal(IndexType name, const DataType &value)
   {
      S32 key = mKeyLookup.find(name)->second;
      AbstractSetting<IndexType> *absSet = mSettings[key];
      TNLAssert((dynamic_cast<Setting<DataType, IndexType> *>(absSet)), "Expected setting!");

      static_cast<Setting<DataType, IndexType> *>(absSet)->setValue(value);
   }


   AbstractSetting<IndexType> *getSetting(IndexType name) const
   {
      TNLAssert(mKeyLookup.find(name) != mKeyLookup.end(), "Setting with specified name not found!");

      return mSettings[mKeyLookup.find(name)->second];
   }


   template <class DataType>
   DataType getVal(IndexType name) const
   {
      AbstractSetting<IndexType> *abstractSetting = getSetting(name);
      TNLAssert((dynamic_cast<Setting<DataType, IndexType> *>(abstractSetting)), "Expected setting!");

      return static_cast<Setting<DataType, IndexType> *>(abstractSetting)->getValue();
   }


   void add(AbstractSetting<IndexType> *setting)
   {
      mSettings.push_back(setting);
      mKeyLookup[setting->getName()] = mSettings.size() - 1;
   }


   string getStrVal(IndexType name) const
   {
      S32 index = mKeyLookup.find(name)->second;
      return mSettings[index]->getValueString();
   }


   string getDefaultStrVal(IndexType name) const
   {
      return mSettings[mKeyLookup.find(name)->second]->getDefaultValueString();
   }


   string getKey(IndexType name) const
   {
      return mSettings[mKeyLookup.find(name)->second]->getKey();
   }


   string getSection(IndexType name) const
   {
      return mSettings[mKeyLookup.find(name)->second]->getSection();
   }


   // There are much more efficient ways to do this!
   Vector<AbstractSetting<IndexType> *> getSettingsInSection(const string &section) const
   {
      Vector<AbstractSetting<IndexType> *> settings;
      for(S32 i = 0; i < mSettings.size(); i++)
      {
         if(mSettings[i]->getSection() == section)
            settings.push_back(mSettings[i]);
      }

      return settings;
   }

};


};

#endif