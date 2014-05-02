//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "StackTracer.h"

#ifdef TNL_OS_WIN32
#  include "StackWalker.h"
#else
#  include <execinfo.h>
#  include <errno.h>
#  include <cxxabi.h>
#endif

#include <stdio.h>
#include <signal.h>


namespace Zap
{

#ifdef TNL_OS_WIN32

static inline void printStackTrace(FILE *out = stderr)
{
   fprintf(out, "Stack Trace:\n");
   StackWalker sw;
   sw.ShowCallstack();
}

#else

static inline void printStackTrace(FILE *out = stderr, U32 max_frames = 63)
{
   fprintf(out, "Stack Trace:\n");
 
   // Storage array for stack trace address data
   void* addrlist[max_frames+1];
 
   // Retrieve current stack addresses
   unsigned int addrlen = backtrace(addrlist, ARRAYSIZE(addrlist));
 
   if(addrlen == 0) 
   {
      fprintf(out, "  \n" );
      return;
   }
 
   // Resolve addresses into strings containing "filename(function+address)",
   // Actually it will be ## program address function + offset
   // this array must be free()-ed
   char** symbollist = backtrace_symbols( addrlist, addrlen );
 
   size_t funcnamesize = 1024;
   char funcname[1024];
 
   // Iterate over the returned symbol lines. skip the first, it is the
   // address of this function.
   for(unsigned int i = 4; i < addrlen; i++)
   {
      char* begin_name   = NULL;
      char* begin_offset = NULL;
      char* end_offset   = NULL;
 
      // Find parentheses and +address offset surrounding the mangled name
#ifdef TNL_OS_MAC_OSX
      // OSX style stack trace
      for(char *p = symbollist[i]; *p; ++p)
      {
         if((*p == '_') && ( *(p-1) == ' '))
            begin_name = p-1;
         else if(*p == '+')
            begin_offset = p-1;
      }
 
      if(begin_name && begin_offset && (begin_name < begin_offset))
      {
         *begin_name++   = '\0';
         *begin_offset++ = '\0';
 
         // mangled name is now in [begin_name, begin_offset) and caller
         // offset in [begin_offset, end_offset). now apply
         // __cxa_demangle():
         S32 status;
         char* ret = abi::__cxa_demangle(begin_name, &funcname[0],
                                         &funcnamesize, &status);
         if(status == 0) 
         {
            funcname = ret; // use possibly realloc()-ed string
            fprintf(out, "  %-30s %-40s %s\n",
                        symbollist[i], funcname, begin_offset);
         } else {
            // demangling failed. Output function name as a C function with
            // no arguments.
            fprintf out, "  %-30s %-38s() %s\n",
                        symbollist[i], begin_name, begin_offset);
         }
 
#else // !TNL_OS_MAC_OSX - but is posix
      // not OSX style
      // ./module(function+0x15c) [0x8048a6d]
      for(char *p = symbollist[i]; *p; ++p)
      {
         if(*p == '(' )
            begin_name = p;
         else if(*p == '+')
            begin_offset = p;
         else if(*p == ')' && ( begin_offset || begin_name ))
            end_offset = p;
      }
 
      if(begin_name && end_offset && (begin_name < end_offset))
      {
         *begin_name++   = '\0';
         *end_offset++   = '\0';
         if(begin_offset)
            *begin_offset++ = '\0';
 
         // mangled name is now in [begin_name, begin_offset) and caller
         // offset in [begin_offset, end_offset). now apply
         // __cxa_demangle():
 
         int status = 0;
         char* ret = abi::__cxa_demangle(begin_name, funcname,
                                         &funcnamesize, &status);
         char* fname = begin_name;
         if(status == 0) 
            fname = ret;
 
         if ( begin_offset )
         {
            fprintf(out, "  %-30s ( %-40s  + %-6s) %s\n",
                     symbollist[i], fname, begin_offset, end_offset);
         } 
         else 
         {
            fprintf(out, "  %-30s ( %-40s    %-6s) %s\n",
                    symbollist[i], fname, "", end_offset);
         }
#endif  // !TNL_OS_MAC_OSX - but is posix
      } else {
         // cCouldn't parse the line? print the whole line.
         fprintf(out, "  %-40s\n", symbollist[i]);
      }
   }
 
   free(symbollist);
}

#endif


static void abortHandler(S32 signum)
{
   // Associate each signal with a signal name string
   const char *name = NULL;

   switch(signum)
   {
      case SIGABRT: name = "SIGABRT";         break;
      case SIGSEGV: name = "SIGSEGV";         break;
#ifndef TNL_OS_WIN32 
      case SIGBUS:  name = "SIGBUS";          break;
#endif
      case SIGILL:  name = "SIGILL";          break;
      case SIGFPE:  name = "SIGFPE";          break;
   }
 
   // Notify the user which signal was caught. We use printf, because this is the 
   // most basic output function. Once you get a crash, it is possible that more 
   // complex output systems like streams and the like may be corrupted. So we 
   // make the most basic call possible to the lowest level, most 
   // standard print function.
   if(name)
      fprintf(stderr, "Caught signal %d (%s)\n", signum, name);
   else
      fprintf(stderr, "Caught signal %d\n", signum);
 
   printStackTrace();
 
   // If you caught one of the above signals, it is likely you just 
   // want to quit your program right now.
   exit(signum);
}


// Constructor
StackTracer::StackTracer()
{
   // SIGABRT is generated when the program calls the abort() function, such as when an assert() triggers.
   signal(SIGABRT, abortHandler); 

   // SIGSEGV and SIGBUS are generated when the program makes an illegal memory access, such as reading unaligned 
   // memory, dereferencing a NULL pointer, reading memory out of bounds etc.  SIGBUS is not supported under Windows.
   signal(SIGSEGV, abortHandler);
#ifndef TNL_OS_WIN32 
   signal(SIGBUS, abortHandler);
#endif

   // SIGILL is generated when the program tries to execute a malformed instruction. This happens when the execution 
   // pointer starts reading non-program data, or when a pointer to a function is corrupted.
   signal(SIGILL, abortHandler);

   // SIGFPE is generated when executing an illegal floating point instruction, most commonly division by zero or 
   // floating point overflow.
   signal(SIGFPE, abortHandler);
}


}