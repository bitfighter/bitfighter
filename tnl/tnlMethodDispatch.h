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

#ifndef _TNL_METHODDISPATCH_H_
#define _TNL_METHODDISPATCH_H_

#include "tnlTypes.h"
#include "tnlVector.h"
#include "tnlBitStream.h"
#include "tnlMethodDispatch.h"
#include "tnlNetStringTable.h"
#include "tnlString.h"

#include "../zap/Point.h"     // Needed for Point specialization with read and functions below

#include <string>

namespace Types
{
   const TNL::U8 VectorSizeBitSize8 = 8;     // CE: Was 8 --> Controls number of bits used to write the length of arrays to
                                 // bitStreams (such as the journal).  Was 8, which allowed for a max array size of 255.  12 bits
                                 // seems to be enough for our local journaling requirements (up to 4096 lines).  Our main constraint on this is that we  
                                 // need enough bits to store the length of a vector containing all the lines from our INI file
                                 // Unfortunately, changing this requires an upgrade of the master server, which will
                                 // basically screw any clients out there at the moment, which I don't want to do right now...
                                 // We'll have to find a way to make the journaling work for us without changing this value!
   const TNL::U8 VectorSizeBitSize16 = 16;   // Vector can send more then 255, and keep compatible to clients that send size of 254 or less.
   const TNL::U8 ByteBufferSizeBitSize = 10;

   /// Reads a string from a BitStream.
   extern void read(TNL::BitStream &s, TNL::StringPtr *val);
   /// Writes a string into a BitStream.
   extern void write(TNL::BitStream &s, TNL::StringPtr &val);
      /// Reads a string from a BitStream.
   extern void read(TNL::BitStream &s, std::string *val);
   /// Writes a string into a BitStream.
   extern void write(TNL::BitStream &s, std::string &val);
   /// Reads a ByteBuffer from a BitStream.
   extern void read(TNL::BitStream &s, TNL::ByteBufferPtr *val);
   /// Writes a ByteBuffer into a BitStream.
   extern void write(TNL::BitStream &s, TNL::ByteBufferPtr &val);
   /// Reads an IP address from a BitStream.
   extern void read(TNL::BitStream &s, TNL::IPAddress *val);
   /// Writes an IP address into a BitStream.
   extern void write(TNL::BitStream &s, TNL::IPAddress &val);

   /// Reads a StringTableEntry from a BitStream.
   inline void read(TNL::BitStream &s, TNL::StringTableEntry *val)
   {
      s.readStringTableEntry(val);
   }
   /// Writes a StringTableEntry into a BitStream.
   inline void write(TNL::BitStream &s, TNL::StringTableEntry &val)
   {
      s.writeStringTableEntry(val);
   }

   /// Reads a bit-compressed RangedU32 from a BitStream.
   template <TNL::U32 MinValue, TNL::U32 MaxValue> inline void read(TNL::BitStream &s, TNL::RangedU32<MinValue,MaxValue> *val)
   {
      val->value = s.readRangedU32(MinValue, MaxValue);
   }
   /// Writes a bit-compressed RangedU32 into a BitStream.
   template <TNL::U32 MinValue, TNL::U32 MaxValue> inline void write(TNL::BitStream &s, TNL::RangedU32<MinValue,MaxValue> &val)
   {
      s.writeRangedU32(val.value, MinValue, MaxValue);
   }

   /// Reads a bit-compressed signed integer from a BitStream.
   template <TNL::U32 BitCount> inline void read(TNL::BitStream &s, TNL::SignedInt<BitCount> *val)
   {
      val->value = s.readSignedInt(BitCount);
   }
   /// Writes a bit-compressed signed integer into a BitStream.
   template <TNL::U32 BitCount> inline void write(TNL::BitStream &s,TNL::SignedInt<BitCount> &val)
   {
      s.writeSignedInt(val.value, BitCount);
   }

   /// Reads a bit-compressed integer from a BitStream
   template <TNL::U32 BitCount> inline void read(TNL::BitStream &s, TNL::Int<BitCount> *val)
   {
      val->value = s.readInt(BitCount);
   }
   /// Writes a bit-compressed integer into a BitStream
   template <TNL::U32 BitCount> inline void write(TNL::BitStream &s,TNL::Int<BitCount> &val)
   {
      s.writeInt(val.value, BitCount);
   }

   /// Reads a bit-compressed Float (0 to 1) from a BitStream.
   template <TNL::U32 BitCount> inline void read(TNL::BitStream &s, TNL::Float<BitCount> *val)
   {
      val->value = s.readFloat(BitCount);
   }
   /// Writes a bit-compressed Float (0 to 1) into a BitStream.
   template <TNL::U32 BitCount> inline void write(TNL::BitStream &s,TNL::Float<BitCount> &val)
   {
      s.writeFloat(val.value, BitCount);
   }

   /// Reads a bit-compressed SignedFloat (-1 to 1) from a BitStream.
   template <TNL::U32 BitCount> inline void read(TNL::BitStream &s, TNL::SignedFloat<BitCount> *val)
   {
      val->value = s.readSignedFloat(BitCount);
   }
   /// Writes a bit-compressed SignedFloat (-1 to 1) into a BitStream.
   template <TNL::U32 BitCount> inline void write(TNL::BitStream &s,TNL::SignedFloat<BitCount> &val)
   {
      s.writeSignedFloat(val.value, BitCount);
   }


/*          // Using template<typename> makes it harder to read errors about missing read() / write().
   /// Reads a generic object from a BitStream.  This can be used for any
   /// type supported by BitStream::read.
   template <typename T> inline void read(TNL::BitStream &s, T *val)
   {
      s.read(val);
   }

   /// Writes a generic object into a BitStream.  This can be used for any
   /// type supported by BitStream::write.  Max data size is 1024 bytes.
   template <typename T> inline void write(TNL::BitStream &s, T &val)
   {
      s.write(val);
   }
*/
   inline void read(TNL::BitStream &s, bool *val)     {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::S8 *val)  {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::S16 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::S32 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::S64 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::U8 *val)  {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::U16 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::U32 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::U64 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::F32 *val) {s.read(val);}
   inline void read(TNL::BitStream &s, TNL::F64 *val) {s.read(val);}

   inline void write(TNL::BitStream &s, bool val)     {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::S8 val)  {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::S16 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::S32 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::S64 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::U8 val)  {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::U16 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::U32 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::U64 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::F32 val) {s.write(val);}
   inline void write(TNL::BitStream &s, TNL::F64 val) {s.write(val);}

/*      // Read and Write string not used?
   /// Read a string object from a BitStream.  (CE)
   inline void read(TNL::BitStream &s, std::string *val)
   {
      char cstr[255];
      s.readString(cstr);
      std::string tmp = cstr;
      *val = tmp;
   }

   /// Write a string object to a Bitstream.  First converts it to a c_string so
   /// we can take advantage of the BitStream::writeString method  (CE)
   inline void write(TNL::BitStream &s, std::string &val)
   {
      char *cstrLine;
      cstrLine = new char[val.substr(0,254).size()+1];       
      strcpy(cstrLine, val.substr(0,254).c_str());                

      s.writeString(cstrLine);      // Max length = 255 bytes

      //delete cstrLine;
   }*/


   const TNL::U32 VectorSizeNumberSize = (1 << VectorSizeBitSize8) - 1;       // 255

   /// Reads a Vector of objects from a BitStream.
   template <typename T> 
   inline void read(TNL::BitStream &s, TNL::Vector<T> *val)
   {
      TNL::U32 size = s.readInt(VectorSizeBitSize8);    // Max 254 -- sending 255 signals that we'll be sending another 2 bytes with larger size
      if(size == VectorSizeNumberSize)                  // Older clients were limited to 255 elements, so we resort to this scheme to remain compatible
         size = s.readInt(VectorSizeBitSize16) + VectorSizeNumberSize;

      val->resize(size);
      for(TNL::S32 i = 0; i < val->size(); i++)
      {
         TNLAssert(s.isValid(), "Error reading vector");
         if(!s.isValid())      // Error, don't read any more!
            break;        

         read(s, &((*val)[i]));
      }
   }


   /// Writes a Vector of objects into a BitStream.
   template <typename T> 
   inline void write(TNL::BitStream &s, TNL::Vector<T> &val)
   {
      if(val.size() >= (TNL::S32)VectorSizeNumberSize)  // Large vector, more than 255 elements
      {
         // Note that if we enter this block, this function will not work with older versions.  If we stay out, it will be compatible.
         TNLAssert((val.size() - VectorSizeNumberSize) < (1 << VectorSizeBitSize16), "Vector too big");

         s.writeInt(VectorSizeNumberSize, VectorSizeBitSize8);
         s.writeInt(val.size() - VectorSizeNumberSize, VectorSizeBitSize16);
      }
      else
         s.writeInt(val.size(), VectorSizeBitSize8);

      for(TNL::S32 i = 0; i < val.size(); i++)
         write(s, val[i]);
   }


   /// Writes a Vector of objects into a BitStream.
   /// allows passing an argument through a vector
   template <typename T, typename A> 
   inline void read(TNL::BitStream &s, TNL::Vector<T> *val, A arg1)
   {
      TNL::U32 size = s.readInt(VectorSizeBitSize8);    // Max 254 -- sending 255 signals that we'll be sending another 2 bytes with larger size
      if(size == VectorSizeNumberSize)                  // Older clients were limited to 255 elements, so we resort to this scheme to remain compatible
         size = s.readInt(VectorSizeBitSize16) + VectorSizeNumberSize;

      val->resize(size);
      for(TNL::S32 i = 0; i < val->size(); i++)
      {
         TNLAssert(s.isValid(), "Error reading vector");
         if(!s.isValid())      // Error, don't read any more!
            break;        

         read(s, &((*val)[i]), arg1);
      }
   }

   /// Writes a Vector of objects into a BitStream.
   template <typename T, typename A> 
   inline void write(TNL::BitStream &s, TNL::Vector<T> &val, A arg1)
   {
      if(val.size() >= (TNL::S32)VectorSizeNumberSize)  // Large vector, more than 255 elements
      {
         // Note that if we enter this block, this function will not work with older versions.  If we stay out, it will be compatible.
         TNLAssert((val.size() - VectorSizeNumberSize) < (1 << VectorSizeBitSize16), "Vector too big");

         s.writeInt(VectorSizeNumberSize, VectorSizeBitSize8);
         s.writeInt(val.size() - VectorSizeNumberSize, VectorSizeBitSize16);
      }
      else
         s.writeInt(val.size(), VectorSizeBitSize8);

      for(TNL::S32 i = 0; i < val.size(); i++)
         write(s, val[i], arg1);
   }
};

namespace TNL {

/// Base class for FunctorDecl template classes.  The Functor objects
/// store the parameters and member function pointer for the invocation
/// of some class member function.  Functor is used in TNL by the
/// RPC mechanism, the journaling system and the ThreadQueue to store
/// a function for later transmission and dispatch, either to a remote
/// host, a journal file, or another thread in the process.
struct Functor {
   /// Construct the Functor.
   Functor() {}
   /// Destruct the Functor.
   virtual ~Functor() {}
   /// Reads this Functor from a BitStream.
   virtual void read(BitStream &stream) = 0;
   /// Writes this Functor to a BitStream.
   virtual void write(BitStream &stream) = 0;
   /// Dispatch the function represented by the Functor.
   virtual void dispatch(Object *t) = 0;
};

/// FunctorDecl template class.  This class is specialized based on the
/// member function call signature of the method it represents.  Other
/// specializations hold specific member function pointers and slots
/// for each of the function arguments.
template <class T>
struct FunctorDecl : public Functor {
   FunctorDecl() {}
   void set() {}
   void read(BitStream &stream) {}
   void write(BitStream &stream) {}
   void dispatch(Object *t) { }
};
template <class T> 
struct FunctorDecl<void (T::*)()> : public Functor {
   typedef void (T::*FuncPtr)();
   FuncPtr ptr;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set() {}
   void read(BitStream &stream) {}
   void write(BitStream &stream) {}
   void dispatch(Object *t) { ((T *)t->*ptr)(); }
}; 
template <class T, class A>
struct FunctorDecl<void (T::*)(A)> : public Functor {
   typedef void (T::*FuncPtr)(A);
   FuncPtr ptr; A a;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a) { a = _a; }
   void read(BitStream &stream) { Types::read(stream, &a); }
   void write(BitStream &stream) { Types::write(stream, a); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a); }
};
template <class T, class A, class B>
struct FunctorDecl<void (T::*)(A,B)>: public Functor {
   typedef void (T::*FuncPtr)(A,B);
   FuncPtr ptr; A a; B b;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b) { a = _a; b = _b;}
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b); }
};

template <class T, class A, class B, class C>
struct FunctorDecl<void (T::*)(A,B,C)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C);
   FuncPtr ptr; A a; B b; C c;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c) { a = _a; b = _b; c = _c;}
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c); }
};

template <class T, class A, class B, class C, class D>
struct FunctorDecl<void (T::*)(A,B,C,D)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D);
   FuncPtr ptr; A a; B b; C c; D d;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d) { a = _a; b = _b; c = _c; d = _d; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d); }
};

template <class T, class A, class B, class C, class D, class E>
struct FunctorDecl<void (T::*)(A,B,C,D,E)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E);
   FuncPtr ptr; A a; B b; C c; D d; E e;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e) { a = _a; b = _b; c = _c; d = _d; e = _e; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e); }
};

template <class T, class A, class B, class C, class D, class E, class F>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k;}
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k); }
};

template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l); }
};


template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L,M)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L,M);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l; M m;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l, M & _m) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; m = _m;}
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); Types::read(stream, &m); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); Types::write(stream, m); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l, m); }
};


template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L,M,N)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L,M,N);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l; M m; N n;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l, M & _m, N & _n) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; m = _m; n = _n; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); Types::read(stream, &m); Types::read(stream, &n); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); Types::write(stream, m); Types::write(stream, n); }
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n); }
};


template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l; M m; N n; O o;
   FunctorDecl(FuncPtr p) : ptr(p) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l, M & _m, N & _n, O & _o) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; m = _m; n = _n; o = _o; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); Types::read(stream, &m); Types::read(stream, &n); Types::read(stream, &o); }
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); Types::write(stream, m); Types::write(stream, n); Types::write(stream, o);}
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o); }
};


template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l; M m; N n; O o;P p;
   FunctorDecl(FuncPtr pt) : ptr(pt) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l, M &_m, N &_n, O &_o, P &_p) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; m = _m; n = _n; o = _o; p = _p; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); Types::read(stream, &m); Types::read(stream, &n); Types::read(stream, &o); Types::read(stream, &p);}
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); Types::write(stream, m); Types::write(stream, n); Types::write(stream, o); Types::write(stream, p);}
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p); }
};


template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l; M m; N n; O o;P p;Q q;
   FunctorDecl(FuncPtr pt) : ptr(pt) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l, M &_m, N &_n, O &_o, P &_p, Q &_q) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; m = _m; n = _n; o = _o; p = _p; q=_q; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); Types::read(stream, &m); Types::read(stream, &n); Types::read(stream, &o); Types::read(stream, &p); Types::read(stream, &q);}
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); Types::write(stream, m); Types::write(stream, n); Types::write(stream, o); Types::write(stream, p); Types::write(stream, q);}
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q); }
};


template <class T, class A, class B, class C, class D, class E, class F, class G, class H, class I, class J, class K, class L, class M, class N, class O, class P, class Q, class R>
struct FunctorDecl<void (T::*)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R)>: public Functor {
   typedef void (T::*FuncPtr)(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R);
   FuncPtr ptr; A a; B b; C c; D d; E e; F f; G g; H h; I i; J j; K k; L l; M m; N n; O o;P p;Q q;R r;
   FunctorDecl(FuncPtr pt) : ptr(pt) {}
   void set(A &_a, B &_b, C &_c, D &_d, E &_e, F &_f, G &_g, H &_h, I &_i, J &_j, K &_k, L &_l, M &_m, N &_n, O &_o, P &_p, Q &_q, R &_r) { a = _a; b = _b; c = _c; d = _d; e = _e; f = _f; g = _g; h = _h; i = _i; j = _j; k = _k; l = _l; m = _m; n = _n; o = _o; p = _p; q=_q; r=_r; }
   void read(BitStream &stream) { Types::read(stream, &a); Types::read(stream, &b); Types::read(stream, &c); Types::read(stream, &d); Types::read(stream, &e); Types::read(stream, &f); Types::read(stream, &g); Types::read(stream, &h); Types::read(stream, &i); Types::read(stream, &j); Types::read(stream, &k); Types::read(stream, &l); Types::read(stream, &m); Types::read(stream, &n); Types::read(stream, &o); Types::read(stream, &p); Types::read(stream, &q); Types::read(stream, &r);}
   void write(BitStream &stream) { Types::write(stream, a); Types::write(stream, b); Types::write(stream, c); Types::write(stream, d); Types::write(stream, e); Types::write(stream, f); Types::write(stream, g); Types::write(stream, h); Types::write(stream, i); Types::write(stream, j); Types::write(stream, k); Types::write(stream, l); Types::write(stream, m); Types::write(stream, n); Types::write(stream, o); Types::write(stream, p); Types::write(stream, q); Types::write(stream, r);}
   void dispatch(Object *t) { (((T *)t)->*ptr)(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r); }
};

};

#endif

