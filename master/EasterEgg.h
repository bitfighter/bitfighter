//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef _EASTEREGG_H_
#define _EASTEREGG_H_

#include "../zap/IniFile.h"
#include "../zap/Settings.h"

#include "tnlBitStream.h"

#include <map>

using namespace TNL;
using namespace Zap;

namespace Master
{


class EasterEgg
{
public:
   // Version 1 - Our first attempt!
   static const U8 CURRENT_VERSION = 1;

private:
   U8 mVersion;

public:
   EasterEgg();
   virtual ~EasterEgg();

   void read(BitStream *s);
   void write(BitStream *s) const;
};

class EasterEggBasket
{
private:
   std::map<S32, EasterEgg> mBasket;
   CIniFile ini;

   EasterEgg *mCurrentEasterEgg;

public:
   EasterEggBasket(const string &iniFile);
   virtual ~EasterEggBasket();

   void loadEasterEggs();
   EasterEgg * getCurrentEasterEgg();
};

}  // namespace Master


// For TNL serialization
namespace Types
{
   extern void read(BitStream &s, Master::EasterEgg *val);
   extern void write(BitStream &s, const Master::EasterEgg &val);
}


#endif /* _EASTEREGG_H_ */
