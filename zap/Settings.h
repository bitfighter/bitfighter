//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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