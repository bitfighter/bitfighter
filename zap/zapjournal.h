//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

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

