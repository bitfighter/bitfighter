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

// These settings represent various INI values and can handle transofrmation of INI strings
// into normal C++ values/enums.  Settings from the INI must have a section, key, value, and comment.
// Settings also have an index by which the C++ code can reference them.
//
// Remember the INI structure:
//    [Section]
//    ; Comment describing Key
//    Key=Value
//
// You can use any type as an index to identify settings; an enum is recommended for clarity and safety.
// Part of the complexity of this class is that it handles coding and decoding of INI values that represent
// enums.  For example, we have a simple enum called YesNo that contains the two values Yes and No.  We
// can use this in the INI to make the settings more readanble, e.g.:
//
//    SaveOnExit=Yes
//
// This code will help translate that string representation of Yes into the enum value Yes, which you can 
// retrieve with a statement like:
//
//    YesNo saveOnExit = settings.getSetting(SaveOnExit);
//    if(saveOnExit == Yes) { save(); }
//
template <class IndexType>
class AbstractSetting
{
private:
   IndexType mIndex;    // Value we use to look this item up
   string mIniKey;      // INI key
   string mIniSection;  // INI section
   string mComment;
   bool mReadOnly;

public:
   // Constructor
   AbstractSetting(IndexType index, const string &key, const string &section, const string &comment):
      mIniKey(key), 
      mIniSection(section), 
      mComment(comment)
   {
      mIndex = index;
      mReadOnly = false;
   }

   virtual ~AbstractSetting() { /* Do nothing */ }      // Destructor

   IndexType getIndex()   const { return mIndex; }           
   string    getKey()     const { return mIniKey; }
   string    getSection() const { return mIniSection; }
   string    getComment() const { return mComment; }


   /////
   // Pure virtual methods, must be implemented by children classes
   /////

   virtual void setValFromString(const string &value) = 0;

   virtual string getValueString() const = 0;         // Return current value, as a string
   virtual string getDefaultValueString() const = 0;  // Return default value, as a string
};


////////////////////////////////////////
////////////////////////////////////////

// This class exists mainly to get past the C++ restriction about partial specialization
// of class methods.  By creating a new class, we can reduce our template parameters
// to one (DataType) from the two we have in Setting (DataType and IndexType).
class Evaluator
{
private:
   //static DisplayMode stringToDisplayMode(string mode);
   static ColorEntryMode stringToColorEntryMode(string mode);
   static GoalZoneFlashStyle stringToGoalZoneFlashStyle(string style);
   static YesNo stringToYesNo(string yesNo);
   static RelAbs stringToRelAbs(string relAbs);

public:
   // Templated declaration
   template <class DataType> DataType fromString(const string &val);

   static string toString(const string &val);
   static string toString(S32 val);
   static string toString(YesNo yesNo);
   static string toString(RelAbs relAbs);
   static string toString(DisplayMode displayMode);
   static string toString(ColorEntryMode colorMode);
   static string toString(GoalZoneFlashStyle flashStyle);
   static string toString(const Color &color);
};


////////////////////////////////////////
////////////////////////////////////////

template <class DataType, class IndexType>
class Setting : public AbstractSetting<IndexType>
{
   typedef AbstractSetting<IndexType> Parent;

private:
   DataType mDefaultValue;
   DataType mValue;
   Evaluator mEvaluator;

public:
   Setting(IndexType index, const DataType &defaultValue, const string &iniKey, const string &iniSection, const string &comment):
      Parent(index, iniKey, iniSection, comment),
      mDefaultValue(defaultValue),
      mValue(defaultValue)
   {
      // Do nothing
   }

   virtual ~Setting() { /* Do nothing */ }

   DataType fromString(const string &val)       { return mEvaluator.fromString<DataType>(val); }

   void setValue(const DataType &value)         { mValue = value;                              }
                                                                                              
   DataType getValue() const                    { return mValue;                               }
   string getValueString() const                { return Evaluator::toString(mValue);          }
   string getDefaultValueString() const         { return Evaluator::toString(mDefaultValue);   }
                                                                                              
   void setValFromString(const string &value)   { setValue(fromString(value));                 }
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
   virtual ~Settings() { mSettings.deleteAndClear(); }  // Destructor


   template <class DataType>
   void setVal(IndexType indexType, const DataType &value)
   {
      S32 key = mKeyLookup.find(indexType)->second;
      AbstractSetting<IndexType> *absSet = mSettings[key];
      TNLAssert((dynamic_cast<Setting<DataType, IndexType> *>(absSet)), "Expected setting!");

      static_cast<Setting<DataType, IndexType> *>(absSet)->setValue(value);
   }


   AbstractSetting<IndexType> *getSetting(IndexType indexType) const
   {
      TNLAssert(mKeyLookup.find(indexType) != mKeyLookup.end(), "Setting with specified index not found!");

      return mSettings[mKeyLookup.find(indexType)->second];
   }


   template <class DataType>
   DataType getVal(IndexType indexType) const
   {
      AbstractSetting<IndexType> *abstractSetting = getSetting(indexType);
      TNLAssert((dynamic_cast<Setting<DataType, IndexType> *>(abstractSetting)), "Expected setting!");

      return static_cast<Setting<DataType, IndexType> *>(abstractSetting)->getValue();
   }


   void add(AbstractSetting<IndexType> *setting)
   {
      mSettings.push_back(setting);
      mKeyLookup[setting->getIndex()] = mSettings.size() - 1;
   }


   string getStrVal(IndexType indexType) const
   {
      S32 index = mKeyLookup.find(indexType)->second;
      return mSettings[index]->getValueString();
   }


   string getDefaultStrVal(IndexType indexType) const
   {
      return mSettings[mKeyLookup.find(indexType)->second]->getDefaultValueString();
   }


   string getKey(IndexType indexType) const
   {
      return mSettings[mKeyLookup.find(indexType)->second]->getKey();
   }


   string getSection(IndexType indexType) const
   {
      return mSettings[mKeyLookup.find(indexType)->second]->getSection();
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


////////////////////////////////////////
////////////////////////////////////////
   
template <typename T>
class EnumParser
{
private:
   typedef map<string, T> ValueMapType;
   ValueMapType mValues;
   Vector<string> mKeys;

public:
    T getVal(const string &value) const
    { 
        ValueMapType::const_iterator iValue = mValues.find(lcase(value));
        if(iValue  == mValues.end())      // Item not found, return default
            return (T)0;

        return iValue->second;
    }


    string getKey(T value) const
    {
      return mKeys[(S32)value];
    }


    void addItem(const string &name, T value) 
    {
      mValues[lcase(name)] = value;

      S32 val = (S32)value;
      
      if(val >= mKeys.size())
         mKeys.resize(val + 1);

      mKeys[val] = name;
    }
};


};

#endif
