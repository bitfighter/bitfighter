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

#ifndef _ZAP_JOURNAL_H_
#define _ZAP_JOURNAL_H_

#include "tnlVector.h"

#include <string>

using namespace TNL;

namespace Zap
{

// These are our "journalable" events... should be able to replay game from these events alone.

class ZapJournal : public Journal
{
public:
   TNL_DECLARE_JOURNAL_ENTRYPOINT(reshape, (S32 newWidth, S32 newHeight));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(motion, (S32 x, S32 y));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(passivemotion, (S32 x, S32 y));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(keydown, (U8 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(keyup, (U8 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(modifierkeydown, (U32 modkey));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(modifierkeyup, (U32 modkey));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(mouse, (S32 button, S32 state, S32 x, S32 y));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(specialkeydown, (S32 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(specialkeyup, (S32 key));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(idle, (U32 timeDelta));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(display, ());
   //TNL_DECLARE_JOURNAL_ENTRYPOINT(readCmdLineParams, (Vector<string> theArgv));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(setINILength, (S32 lineCount));
   TNL_DECLARE_JOURNAL_ENTRYPOINT(processINILine, (TNL::StringPtr iniLine));

};

}

#endif

