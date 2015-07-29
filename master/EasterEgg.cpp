//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "EasterEgg.h"


namespace Master
{

EasterEggBasket::EasterEggBasket(const string &iniFile)
{
   ini.SetPath(iniFile);

   mCurrentEasterEgg = NULL;
}


EasterEggBasket::~EasterEggBasket()
{
   // Do nothing
}


void EasterEggBasket::loadEasterEggs()
{
   if(ini.getPath() == "")
      return;

   // Clear, then read
   ini.Clear();
   ini.ReadFile();

   // TODO
   // 1. Load EasterEgg data from easteregg.ini into map
   //    I Figure the data would look something like this:
   //       [Christmas]
   //       Enable=true
   //       Time=1225
   //       MainMenuGraphicFile=some_custom_data_file_with_maybe_snowflakes
   //             (with GL data? or PNG?)
   //       MainMenuAction=Falling
   //       MenuHighlightColor=Red
   //
   // 2. Determine if today's date matches an EasterEgg.  If not, set
   //    mCurrentEasterEgg = NULL
   //
   // 3. If we have an easter egg, load the data into an EasterEgg object and
   //    set mCurrentEasterEgg to it, ready for sending to clients.
   //
}


EasterEgg *EasterEggBasket::getCurrentEasterEgg()
{
   return mCurrentEasterEgg;
}


////////////////////////////////////
////////////////////////////////////

EasterEgg::EasterEgg()
{
   mVersion = 0;  // Doesn't exist
}


EasterEgg::~EasterEgg()
{
   // Do nothing
}


// Read from the bitstream and into this EasterEgg object
void EasterEgg::read(BitStream *s)
{
   // Version comes first
   s->read(&mVersion);  // U8
}


// Write this EasterEgg into the bitstream
void EasterEgg::write(BitStream *s) const
{
   s->write(CURRENT_VERSION);
}


} // namespace Master


namespace Types
{

void read(TNL::BitStream &s, Master::EasterEgg *val)
{
   val->read(&s);
}


void write(TNL::BitStream &s, const Master::EasterEgg &val)
{
   val.write(&s);
}

} // namespace Types
