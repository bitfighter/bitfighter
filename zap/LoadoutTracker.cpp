//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------


#include "LoadoutTracker.h"
#include "statistics.h"
#include "stringUtils.h"      // For parseString
#include "tnlBitStream.h"

#include "tnlLog.h"


namespace Zap
{


bool DesignTracker::operator != (const DesignTracker &other) const
{
   return !(*this == other);
}


// Constructor
LoadoutTracker::LoadoutTracker()
{
   reset();
}


// Constructor
LoadoutTracker::LoadoutTracker(const string &loadoutStr)
{
   reset();
   set(loadoutStr);
}


// Constructor
LoadoutTracker::LoadoutTracker(const Vector<U8> &loadout)
{
   reset();
   setLoadout(loadout);
}


// Destructor
LoadoutTracker::~LoadoutTracker()
{
   // Do nothing
}


// Reset this loadout to its factory settings
void LoadoutTracker::reset()
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      mModules[i] = ModuleNone;

   for(S32 i = 0; i < ShipWeaponCount; i++)
      mWeapons[i] = WeaponNone;

   deactivateAllModules();
   mActiveWeapon = 0;
}


// Returns true if anything changed
bool LoadoutTracker::update(const LoadoutTracker &loadout)
{
   bool loadoutChanged = false;

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(mModules[i] != loadout.mModules[i])
      {
         mModules[i] = loadout.mModules[i];
         loadoutChanged = true;
      }

   for(S32 i = 0; i < ModuleCount; i++)
   {
      mModulePrimaryActive[i]   = loadout.mModulePrimaryActive[i];  
      mModuleSecondaryActive[i] = loadout.mModuleSecondaryActive[i];
   }

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(mWeapons[i] != loadout.mWeapons[i])
      {
         mWeapons[i] = loadout.mWeapons[i];
         loadoutChanged = true;
      }

   return loadoutChanged;
}


void LoadoutTracker::writeToStream(BitStream *stream)
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      stream->writeEnum(getModule(i), ModuleCount);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      stream->writeEnum(getWeapon(i), WeaponCount);
}


void LoadoutTracker::readFromStream(BitStream *stream)
{
   // Update loadout
   for(S32 i = 0; i < ShipModuleCount; i++)
      setModule(i, (ShipModule)stream->readEnum(ModuleCount));

   for(S32 i = 0; i < ShipWeaponCount; i++)
      setWeapon(i, (WeaponType)stream->readEnum(WeaponCount));
}


// Takes a vector of U8s repesenting loadout... M,M,W,W,W
void LoadoutTracker::setLoadout(const Vector<U8> &items)
{
   reset();

   // Check for the proper number of items
   if(items.size() != ShipModuleCount + ShipWeaponCount)
      return;

   // Do some range checking
   for(S32 i = 0; i < ShipModuleCount; i++)
      if(items[i] >= ModuleCount)
         return;

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(items[i + ShipModuleCount] >= WeaponCount)
         return;

   // If everything checks out, we can fill the loadout
   for(S32 i = 0; i < ShipModuleCount; i++)
      mModules[i] = (ShipModule) items[i];

   for(S32 i = 0; i < ShipWeaponCount; i++)
      mWeapons[i] = (WeaponType) items[i + ShipModuleCount];
}


void LoadoutTracker::set(const string &loadoutStr)
{
   // If we have a loadout string, try to get something useful out of it.
   // Note that even if we are able to parse the loadout successfully, it might still be invalid for a 
   // particular server or gameType... engineer, for example, is not allowed everywhere.
   if(loadoutStr == "")
      return;

   Vector<string> words;
   parseString(loadoutStr, words, ',');

   if(words.size() != ShipModuleCount + ShipWeaponCount)      // Invalid loadout string
   {
      logprintf(LogConsumer::ConfigurationError, "Misconfigured loadout preset found in INI");
      return;
   }

   bool found;

   for(S32 i = 0; i < ShipModuleCount; i++)
   {
      found = false;
      const char *word = words[i].c_str();

      for(S32 j = 0; j < ModuleCount; j++)
         if(stricmp(word, ModuleInfo::getModuleInfo((ShipModule) j)->getName()) == 0)     // Case insensitive
         {
            mModules[i] = ShipModule(j);
            found = true;
            break;
         }

      if(!found)
      {
         logprintf(LogConsumer::ConfigurationError, "Unknown module found in loadout preset in INI file: %s", word);
         reset();
         return;
      }
   }

   for(S32 i = 0; i < ShipWeaponCount; i++)
   {
      found = false;
      const char *word = words[i + ShipModuleCount].c_str();

      for(S32 j = 0; j < WeaponCount; j++)
         if(stricmp(word, WeaponInfo::getWeaponInfo(WeaponType(j)).name.getString()) == 0)
         {
            mWeapons[i] = WeaponType(j);
            found = true;
            break;
         }

      if(!found)
      {
         logprintf(LogConsumer::ConfigurationError, "Unknown weapon found in loadout preset in INI file: %s", word);
         reset();
         return;
      }
   }
}


void LoadoutTracker::setModule(U32 moduleIndex, ShipModule module)
{
   mModules[moduleIndex] = module;
}


void LoadoutTracker::setWeapon(U32 weaponIndex, WeaponType weapon)
{
   mWeapons[weaponIndex] = weapon;
}


void LoadoutTracker::setActiveWeapon(U32 weaponIndex)
{
   mActiveWeapon = weaponIndex % ShipWeaponCount;
}


void LoadoutTracker::setModulePrimary(ShipModule module, bool isActive)
{
   mModulePrimaryActive[module] = isActive;
}


void LoadoutTracker::setModuleIndexPrimary(U32 moduleIndex, bool isActive)
{
   mModulePrimaryActive[mModules[moduleIndex]] = isActive;
}


void LoadoutTracker::setModuleSecondary(ShipModule module, bool isActive)
{
    mModuleSecondaryActive[module] = isActive;
}


void LoadoutTracker::setModuleIndexSecondary(U32 moduleIndex, bool isActive)
{
   mModuleSecondaryActive[mModules[moduleIndex]] = isActive;
}


void LoadoutTracker::deactivateAllModules()
{
   for(S32 i = 0; i < ModuleCount; i++)
   {
      mModulePrimaryActive[i] = false;
      mModuleSecondaryActive[i] = false;
   }
}


bool LoadoutTracker::hasModule(ShipModule mod) const
{
   for(S32 i = 0; i < ShipModuleCount; i++)
      if(mModules[i] == mod)
         return true;

   return false;
}


bool LoadoutTracker::hasWeapon(WeaponType weapon) const
{
   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(mWeapons[i] == weapon)
         return true;

   return false;
}


// Minimal validity test; basically seeing if it has been set or not.
bool LoadoutTracker::isValid() const
{
   return mModules[0] != ModuleNone;
}


// Is this design valid for the current game?  Design comes from the client, so might be Sneaky Pete.
bool LoadoutTracker::isValidForLevel(bool engineerAllowed) const
{
   if(!isValid())
      return false;

   // Reject if module contains engineer but it is not enabled on this level
   if(!engineerAllowed && hasModule(ModuleEngineer))
      return false;

   // Check for illegal weapons
   if(hasWeapon(WeaponTurret))
      return false;

   return true;     // Passed validation
}


bool LoadoutTracker::isWeaponActive(U32 weaponIndex) const
{
   return weaponIndex == mActiveWeapon;
}


WeaponType LoadoutTracker::getWeapon(U32 weaponIndex) const
{
   return mWeapons[weaponIndex];
}


WeaponType LoadoutTracker::getActiveWeapon() const
{
   return mWeapons[mActiveWeapon];
}


U32 LoadoutTracker::getActiveWeaponIndex() const
{
   return mActiveWeapon;
}


ShipModule LoadoutTracker::getModule(U32 modIndex) const
{
   TNLAssert(modIndex < (U32)ShipModuleCount, "Invalid modIndex!");
   return mModules[modIndex];
}


bool LoadoutTracker::isModulePrimaryActive(ShipModule module) const
{
   return mModulePrimaryActive[module];
}


bool LoadoutTracker::isModuleSecondaryActive(ShipModule module) const
{
   return mModuleSecondaryActive[module];
}


Vector<U8> LoadoutTracker::pack() const
{
   Vector<U8> loadout(ShipModuleCount + ShipWeaponCount);

   for(S32 i = 0; i < ShipModuleCount; i++)
      loadout.push_back(U8(mModules[i]));

   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadout.push_back(U8(mWeapons[i]));

   return loadout;
}


bool LoadoutTracker::operator == (const DesignTracker &other) const 
{
   const LoadoutTracker *o = dynamic_cast<const LoadoutTracker *>(&other);
   TNLAssert(o, "Comparing LoadoutTracker with non-LoadoutTracker!");

   if(!o)
      return false;

   for(S32 i = 0; i < ShipModuleCount; i++)
      if(getModule(i) != o->getModule(i))
         return false;

   for(S32 i = 0; i < ShipWeaponCount; i++)
      if(getWeapon(i) != o->getWeapon(i))
         return false;

   return true;
}


// Pass compact == true to squeeze spaces out of string, false to make string prettier
string LoadoutTracker::toString(bool compact) const
{
   if(!isValid())
      return compact ? "" : "<< Undefined >>";

   Vector<string> loadoutStrings(ShipModuleCount + ShipWeaponCount);    // Reserve space for efficiency

   // First modules
   for(S32 i = 0; i < ShipModuleCount; i++)
      loadoutStrings.push_back(ModuleInfo::getModuleInfo((ShipModule) mModules[i])->getName());

   // Then weapons
   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadoutStrings.push_back(WeaponInfo::getWeaponInfo(mWeapons[i]).name.getString());

   return listToString(loadoutStrings, compact ? "," : ", ");
}


void LoadoutTracker::saveDesignStats(Statistics &statistics) const
{
   // This builds a loadout 'hash' by devoting the first 16 bits to modules, the
   // second 16 bits to weapons.  The integer created might look like so:
   //    00000000000001110000000000000011
   U32 loadoutHash = 0;

   for(S32 i = 0; i < ShipModuleCount; i++)
      loadoutHash |= BIT(hasModule(ShipModule(i)) ? 1 : 0);

   for(S32 i = 0; i < ShipWeaponCount; i++)
      loadoutHash |= BIT(hasWeapon(WeaponType(i)) ? 1 : 0) << 16;

   statistics.addLoadout(loadoutHash);
}


}
