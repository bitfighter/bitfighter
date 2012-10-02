//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef _TNL_TYPES_H_
#define _TNL_TYPES_H_


//--------------------------------------
// Enable Asserts in all debug builds
#if defined(TNL_DEBUG)
#ifndef TNL_ENABLE_ASSERTS
#define TNL_ENABLE_ASSERTS
#endif
#endif
#include <stdlib.h>
//inline void* operator new(size_t size, void* ptr) { return ptr; }
#include <new>

//------------------------------------------------------------------------------
//-------------------------------------- Basic Types...
namespace TNL {

/// @defgroup BasicTypes Basic Compiler Independent Types
/// These types are defined so that we know exactly what we have, sign and bit wise.
///
/// The number represents number of bits, the letters represent <b>S</b>igned,
/// <b>U</b>nsigned, or <b>F</b>loating point (implicitly signed).
/// @{

typedef signed char         S8;      ///< Compiler independent signed char (8bit integer).
typedef unsigned char       U8;      ///< Compiler independent unsigned char (8bit integer).

typedef signed short        S16;     ///< Compiler independent signed 16-bit short integer.
typedef unsigned short      U16;     ///< Compiler independent unsigned 16-bit short integer.

typedef signed int          S32;     ///< Compiler independent signed 32-bit integer.
typedef unsigned int        U32;     ///< Compiler independent unsigned 32-bit integer.

typedef float               F32;     ///< Compiler independent 32-bit float.
typedef double              F64;     ///< Compiler independent 64-bit float.

/// @}


#ifndef NULL
#  define NULL 0
#endif
/// NetType serves as a base class for all bit-compressed versions of
/// the base types that can be transmitted using TNL's RPC mechanism.
/// In general, the type names are self-explanatory, providing simple
/// wrappers on the original base types.  The template argument for bit
/// counts or numeric ranges is necessary because TNL parses the actual
/// function prototype as a string in order to determine how many bits
/// to use for each RPC parameter.
///
/// Template parameters to the NetType templates can be either integer
/// constants or enumeration values.  If enumeration values are used,
/// the TNL_DECLARE_RPC_ENUM or TNL_DECLARE_RPC_MEM enum macros must
/// be used to register the enumerations with the RPC system.
struct NetType {
   /* Intentionally empty */
};

/// Unsigned integer bit-level template wrapper.
///
/// When an Int<X> is in the parameter list for an RPC method, that parameter will
/// be transmitted using X bits.
template<U32 bitCount> struct Int : NetType
{
   U32 value;
   Int(U32 val=0) { value = val; }
   operator U32() const { return value; }
   U32 getPrecisionBits() { return bitCount; }
};

/// Signed integer bit-level template wrapper.
///
/// When a SignedInt<X> is in the parameter list for an RPC method, that parameter will
/// be transmitted using X bits.
template<U32 bitCount> struct SignedInt : NetType
{
   S32 value;
   SignedInt(S32 val=0) { value = val; }
   operator S32() const { return value; }
   U32 getPrecisionBits() { return bitCount; }
};

/// Floating point 0...1 value bit-level template wrapper.
///
/// When a Float<X> is in the parameter list for an RPC method, that parameter will
/// be transmitted using X bits.
template<U32 bitCount> struct Float : NetType
{
   F32 value;
   Float(F32 val=0) { value = val; }
   operator F32() const { return value; }
   U32 getPrecisionBits() { return bitCount; }
};

/// Floating point -1...1 value bit-level template wrapper.
///
/// When a SignedFloat<X> is in the parameter list for an RPC method, that parameter will
/// be transmitted using X bits.
template<U32 bitCount> struct SignedFloat : NetType
{
   F32 value;
   SignedFloat(F32 val=0) { value = val; }
   operator F32() const { return value; }
   U32 getPrecisionBits() { return bitCount; }
};

/// Unsigned ranged integer bit-level template wrapper.
///
/// The RangedU32 is used to specify a range of valid values for the parameter
/// in the parameter list for an RPC method.
template<U32 rangeStart, U32 rangeEnd> struct RangedU32 : NetType
{
   U32 value;
   RangedU32(U32 val=rangeStart) { value = val; }
   operator U32() const { return value; }
};


//----------------------------------------------------------------------------------
// Identify the compiler and OS specific stuff we need:
//----------------------------------------------------------------------------------

#if defined (_MSC_VER)

typedef signed _int64   S64;
typedef unsigned _int64 U64;


#define TNL_COMPILER_VISUALC _MSC_VER

#if _MSC_VER < 1200
   // No support for old compilers
#  error "VC: Minimum Visual C++ 6.0 or newer required"
#else  //_MSC_VER >= 1200
#  define TNL_COMPILER_STRING "VisualC++"
#endif

#if _MSC_VER < 1200
#define for if(false) {} else for   ///< Hack to work around Microsoft VC's non-C++ compliance on variable scoping
#endif                              // appears to be fixed on recent version of compiler

#ifdef _MSC_VER
// disable warning caused by memory layer
// see msdn.microsoft.com "Compiler Warning (level 1) C4291" for more details
#pragma warning(disable: 4291)
// disable performance warning of integer to bool conversions
#pragma warning(disable: 4800)
#endif

#elif defined(__MWERKS__) && defined(_WIN32)

typedef signed long long    S64;  ///< Compiler independent signed 64-bit integer
typedef unsigned long long  U64;  ///< Compiler independent unsigned 64-bit integer

#define TNL_COMPILER_STRING "Metrowerks CW Win32"

#elif defined(__GNUC__)

typedef signed long long    S64;  ///< Compiler independent signed 64-bit integer
typedef unsigned long long  U64;  ///< Compiler independent unsigned 64-bit integer

#if defined(__MINGW32__)
#  define TNL_COMPILER_STRING "GCC (MinGW)"
#  define TNL_COMPILER_MINGW
#elif defined(__CYGWIN__)
#  define TNL_COMPILER_STRING "GCC (Cygwin)"
#  define TNL_COMPILER_MINGW
#else
#  define TNL_COMPILER_STRING "GCC "
#endif

#else
#  error "TNL: Unknown Compiler"
#endif



//------------------------------------------------------------------------------
//-------------------------------------- Type constants...

/// @defgroup BasicConstants Global Constants
///
/// Handy constants!
/// @{

#define __EQUAL_CONST_F F32(0.000001)                            ///< Constant float epsilon used for F32 comparisons

static const F32 FloatOne  = F32(1.0);                           ///< Constant float 1.0
static const F32 FloatHalf = F32(0.5);                           ///< Constant float 0.5
static const F32 FloatZero = F32(0.0);                           ///< Constant float 0.0

static const F32 FloatPi   = F32(3.14159265358979323846);            ///< Constant float PI
static const F32 Float2Pi  = F32(2.0 * 3.14159265358979323846);      ///< Constant float 2*PI
static const F32 FloatTau  = Float2Pi;                               ///< For raptor
static const F32 FloatInversePi = F32(1.0 / 3.14159265358979323846); ///< Constant float 1 / PI
static const F32 FloatHalfPi = F32(0.5 * 3.14159265358979323846);    ///< Constant float 1/2 * PI
static const F32 Float2InversePi = F32(2.0 / 3.14159265358979323846);///< Constant float 2 / PI
static const F32 FloatInverse2Pi = F32(0.5 / 3.14159265358979323846);///< Constant float 1 / 2PI

static const F32 FloatSqrt2 = F32(1.41421356237309504880f);          ///< Constant float sqrt(2)
static const F32 FloatSqrtHalf = F32(0.7071067811865475244008443f);  ///< Constant float sqrt(0.5)

static const S8  S8_MIN  = S8(-128);                              ///< Constant Min Limit S8
static const S8  S8_MAX  = S8(127);                               ///< Constant Max Limit S8
static const U8  U8_MAX  = U8(0xFF);                              ///< Constant Max Limit U8

static const S16 S16_MIN = S16(-32768);                           ///< Constant Min Limit S16
static const S16 S16_MAX = S16(32767);                            ///< Constant Max Limit S16
static const U16 U16_MAX = U16(0xFFFF);                           ///< Constant Max Limit U16

static const S32 S32_MIN = S32(-2147483647 - 1);                  ///< Constant Min Limit S32
static const S32 S32_MAX = S32(2147483647);                       ///< Constant Max Limit S32
static const U32 U32_MAX = U32(0xFFFFFFFF);                       ///< Constant Max Limit U32

static const F32 F32_MIN = F32(1.175494351e-38F);                 ///< Constant Min Limit F32
static const F32 F32_MAX = F32(3.402823466e+38F);                 ///< Constant Max Limit F32

static const S64 S64_MIN = S64(0x8000000000000000LL);             ///< Constant Min Limit S64
static const S64 S64_MAX = S64(0x7FFFFFFFFFFFFFFFLL);             ///< Constant Max Limit S64
static const U64 U64_MAX = U64(0xFFFFFFFFFFFFFFFFULL);            ///< Constant Max Limit U64




//----------------------------------------------------------------------------------
// Identify the target Operating System
//----------------------------------------------------------------------------------

#if defined (__ANDROID__)
#  define TNL_OS_STRING "Android"
#  define TNL_OS_ANDROID
#  define TNL_OS_MOBILE
#  define TNL_OS_LINUX
#  define FN_CDECL

#elif defined(__WIN32__) || defined(_WIN32) || defined(__CYGWIN__)
#  define TNL_OS_STRING "Win32"
#  define TNL_OS_WIN32

#ifdef TNL_COMPILER_MINGW
#  define FN_CDECL
#else
#  define FN_CDECL __cdecl
#endif

#elif defined(linux)
#  define TNL_OS_STRING "Linux"
#  define TNL_OS_LINUX
#  define FN_CDECL

#elif defined(__OpenBSD__)
#  define TNL_OS_STRING "OpenBSD"
#  define TNL_OS_OPENBSD
#  define FN_CDECL

#elif defined(__FreeBSD__)
#  define TNL_OS_STRING "FreeBSD"
#  define TNL_OS_FREEBSD
#  define FN_CDECL

#elif defined(__APPLE__)
#include "TargetConditionals.h"
#  if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#    define TNL_OS_STRING "iOS"
#    define TNL_OS_IPHONE
#    define TNL_OS_MOBILE
#    define __IPHONEOS__ // needed by SDL headers
//#    define TNL_OS_MAC_OSX // different platform
#  else
#    define TNL_OS_STRING "MacOSX"
#    define TNL_OS_MAC_OSX
#  endif
#  define FN_CDECL

#else
#  error "TNL: Unsupported Operating System"
#endif



//----------------------------------------------------------------------------------
// Identify the target CPU and assembly language options
//----------------------------------------------------------------------------------


// Other values that might be needed here are: 
// defined(_M_AMD64)|| defined(__amd64__) || defined(_M_IA64) || defined(__amd64) || defined(__ia64__) || defined(_IA64) || defined(__IA64__) || defined(__i686__)
#if defined(_M_IX86) || defined(i386) || defined(__x86_64__) || defined(__x86_64__) || defined(__x86_64)
#  define TNL_CPU_STRING "Intel x86"
#  define TNL_CPU_X86
#  define TNL_LITTLE_ENDIAN
#  define TNL_SUPPORTS_NASM

#  if defined (__GNUC__)
#    if __GNUC__ == 2
#      define TNL_GCC_2
#    elif __GNUC__ == 3
#      define TNL_GCC_3
#    elif __GNUC__ == 4
#      define TNL_GCC_4
#    else
#      error "TNL: Unsupported version of GCC (see tnlMethodDispatch.cpp)"
#    endif
#    define TNL_SUPPORTS_GCC_INLINE_X86_ASM
#  elif defined (__MWERKS__)
#    define TNL_SUPPORTS_MWERKS_INLINE_X86_ASM
#  else
#    define TNL_SUPPORTS_VC_INLINE_X86_ASM
#  endif

#elif defined(__ppc__)
#  define TNL_CPU_STRING "PowerPC"
#  define TNL_CPU_PPC
#  define TNL_BIG_ENDIAN
#  ifdef __GNUC__
#    define TNL_SUPPORTS_GCC_INLINE_PPC_ASM
#  endif
#elif defined(__arm__)
#  define TNL_CPU_STRING "ARM"
#  define TNL_CPU_ARM
#  define TNL_LITTLE_ENDIAN
#  define TNL_GCC_4
#else
#  error "TNL: Unsupported Target CPU"
#endif


/// @}

///@defgroup ObjTrickery Object Management Trickery
///
/// These functions are to construct and destruct objects in memory
/// without causing a free or malloc call to occur. This is so that
/// we don't have to worry about allocating, say, space for a hundred
/// NetAddresses with a single malloc call, calling delete on a single
/// NetAdress, and having it try to free memory out from under us.
///
/// @{

/// Constructs an object that already has memory allocated for it.
template <class T>
inline T* constructInPlace(T* p)
{
   return new(p) T;
}

/// Copy constructs an object that already has memory allocated for it.
template <class T>
inline T* constructInPlace(T* p, const T* copy)
{
   return new(p) T(*copy);
}

/// Destructs an object without freeing the memory associated with it.
template <class T>
inline void destructInPlace(T* p)
{
   p->~T();    // Sometimes crashes here in editor...  does it do it often? -CE
}

/// @}

/// @name GeneralMath Math Helpers
///
/// Some general numeric utility functions.
///
/// @{

/// Determines if number is a power of two.
inline bool isPow2(const U32 number)
{
   return (number & (number - 1)) == 0;
}

///// Determines the binary logarithm of the input value rounded down to the nearest power of 2.
//inline U32 getBinLog2Bad(U32 value)
//{
//   F32 floatValue = (F32)(value);
//   return (*((U32 *) &floatValue) >> 23) - 127;
//}


static const char LogTable256[256] = 
{
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

// From http://www-graphics.stanford.edu/~seander/bithacks.html#IntegerLogLookup
// Replaces original TNL function which caused warnings under higher optimizer settings
inline U32 getBinLog2(U32 value)
{
   register U32 t, tt; // temporaries
   tt = value >> 16;
   if(tt != 0)
     return (t = tt >> 8) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
   else 
     return (t = value >> 8) ? 8 + LogTable256[t] : LogTable256[value];
}




/// Determines the binary logarithm of the next greater power of two of the input number.
inline U32 getNextBinLog2(U32 number)
{
   return getBinLog2(number) + (isPow2(number) ? 0 : 1);
}

/// Determines the next greater power of two from the value.  If the value is a power of two, it is returned.
inline U32 getNextPow2(U32 value)
{
   return isPow2(value) ? value : (1 << (getBinLog2(value) + 1));
}


/// @defgroup MinMaxFuncs Many version of min and max
///
/// We can't use template functions because MSVC6 chokes.
///
/// So we have these...
/// @{

#define DeclareTemplatizedMinMax(type) \
 inline type getMin(type a, type b) { return a > b ? b : a; } \
 inline type getMax(type a, type b) { return a > b ? a : b; }

DeclareTemplatizedMinMax(U32)
DeclareTemplatizedMinMax(S32)
DeclareTemplatizedMinMax(U16)
DeclareTemplatizedMinMax(S16)
DeclareTemplatizedMinMax(U8)
DeclareTemplatizedMinMax(S8)
DeclareTemplatizedMinMax(F32)
DeclareTemplatizedMinMax(F64)

/// @}

inline void writeU32ToBuffer(U32 value, U8 *buffer)
{
   buffer[0] = value >> 24;
   buffer[1] = value >> 16;
   buffer[2] = value >> 8;
   buffer[3] = value;
}

inline U32 readU32FromBuffer(const U8 *buf)
{
   return (U32(buf[0]) << 24) |
          (U32(buf[1]) << 16) |
          (U32(buf[2]) << 8 ) |
          U32(buf[3]);
}

inline void writeU16ToBuffer(U16 value, U8 *buffer)
{
   buffer[0] = value >> 8;
   buffer[1] = (U8) value;
}

inline U16 readU16FromBuffer(const U8 *buffer)
{
   return (U16(buffer[0]) << 8) |
          U16(buffer[1]);
}

inline U32 fourByteAlign(U32 value)
{
   return (value + 3) & ~3;
}

#define BIT(x) (1 << (x))                       ///< Returns value with bit x set (2^x)

};

#endif //_TNL_TYPES_H_
