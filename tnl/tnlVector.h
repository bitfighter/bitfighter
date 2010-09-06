//-----------------------------------------------------------------------------------
//
//   Torque Network Library
//   Copyright (C) 2004 GarageGames.com, Inc.
//   Modifications (C) 2008 Chris Eykamp
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

#ifndef _TNL_VECTOR_H_
#define _TNL_VECTOR_H_

//Includes
#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#ifndef _TNL_PLATFORM_H_
#include "tnlPlatform.h"
#endif

#ifndef _TNL_ASSERT_H_
#include "tnlAssert.h"
#endif

#define VectorBlockSize 16

namespace TNL {

//-----------------------------------------------------------------------------

/// VectorRep is an image of a Vector template object that is used
/// for marshalling and unmarshalling Vectors across RPCs.
struct VectorRep
{
   U32 elementCount;
   U32 arraySize;
   U8 *array;
};

// =============================================================================

/// A dynamic array template class.
///
/// The vector grows as you insert or append
/// elements.  Insertion is fastest at the end of the array.  Resizing
/// of the array can be avoided by pre-allocating space using the
/// reserve() method.
template<class T> class Vector
{
  protected:
   U32 mElementCount; ///< Number of elements currently in the Vector.
   U32 mArraySize;    ///< Number of elements allocated for the Vector.
   T*  mArray;        ///< Pointer to the Vector elements.

   void  checkSize(U32 newElementCount);///< checks the element count against the array size and resizes the array if necessary
   void  destroy(U32 start, U32 end);   ///< Destructs elements from <i>start</i> to <i>end-1</i>
   void  construct(U32 start, U32 end); ///< Constructs elements from <i>start</i> to <i>end-1</i>
   void  construct(U32 start, U32 end, const T* array);
  public:
   Vector(const U32 initialSize = 0);     // Constructor
   Vector(const Vector&);
   ~Vector();

   /// @name VectorSTL STL interface
   ///
   /// @{

   ///
   typedef T        value_type;
   typedef T&       reference;
   typedef const T& const_reference;

   typedef S32      difference_type;
   typedef U32      size_type;

   typedef difference_type (QSORT_CALLBACK *compare_func)(T *a, T *b);

   Vector<T>& operator=(const Vector<T>& p);

   S32 size() const;
   bool empty() const;

   T&       get(S32 index);
   const T& get(S32 index) const;
   T&       get(U32 index) { return get(S32(index)); }

   T&       front();
   const T& front() const;
   T&       back();
   const T& back() const;

   void push_front(const T&);
   void push_back(const T&);
   T& pop_front();
   T& pop_back();

   T& operator[](U32);
   const T& operator[](U32) const;

   T& operator[](S32 i)              { return operator[](U32(i)); }
   const T& operator[](S32 i ) const { return operator[](U32(i)); }

   void reserve(U32);

   /// @}

   /// @name VectorExtended Extended Interface
   ///
   /// @{

   ///
   U32  memSize() const;
   T*   address() const;
   U32  setSize(U32);
   void insert(U32);
   void erase(U32);
   void erase_fast(U32);
   void clear();
   void deleteAndClear();
   void compact();

   void sort(compare_func f);
   T& first();
   T& last();
   const T& first() const;
   const T& last() const;

   void set(void * addr, U32 sz);

   /// @}
};

template<class T> inline Vector<T>::~Vector()                        // Destructor
{
   destroy(0, mElementCount);
   free(mArray);
}

template<class T> inline Vector<T>::Vector(const U32 initialSize)   // Constructor
{
   mArray        = 0;
   mElementCount = 0;
   mArraySize    = 0;
   if(initialSize)
      reserve(initialSize);
}

template<class T> inline Vector<T>::Vector(const Vector& p)        // Copy constructor
{
   mArray = 0;
   mArraySize = 0;
   mElementCount = 0;

   checkSize(p.mElementCount);
   mElementCount = p.mElementCount;
   construct(0, p.mElementCount, p.mArray);
}


template<class T> inline void  Vector<T>::destroy(U32 start, U32 end) // destroys from start to end-1
{
   while(start < end)
   {
      destructInPlace(&mArray[start]);
      start++;
   }
}

template<class T> inline void  Vector<T>::construct(U32 start, U32 end) // destroys from start to end-1
{
   while(start < end)
   {
      constructInPlace(&mArray[start]);
      start++;
   }
}

template<class T> inline void  Vector<T>::construct(U32 start, U32 end, const T* array) // destroys from start to end-1
{
   while(start < end)
   {
      constructInPlace(&mArray[start], &array[start]);
      start++;
   }
}

template<class T> inline T* Vector<T>::address() const
{
   return mArray;
}

template<class T> inline U32 Vector<T>::setSize(U32 size)
{
   checkSize(size);

   if(size > mElementCount)
   {
      construct(mElementCount, size);
      mElementCount = size;
   }
   else if(size < mElementCount)
   {
      destroy(size, mElementCount);
      mElementCount = size;
      if(!mElementCount)
      {
         free(mArray);
         mArray = NULL;
         mArraySize = 0;
      }
   }
   return mElementCount;
}

template<class T> inline void Vector<T>::insert(U32 index)
{
   checkSize(mElementCount + 1);
   constructInPlace(&mArray[mElementCount]);
   mElementCount++;

   for(U32 i = mElementCount - 1; i > index; i--)
      mArray[i] = mArray[i - 1];
   destructInPlace(&mArray[index]);
   constructInPlace(&mArray[index]);
}

template<class T> inline void Vector<T>::erase(U32 index)
{
   // Assert: index >= 0 && index < mElementCount
   for(U32 i = index; i < mElementCount - 1; i++)
      mArray[i] = mArray[i+1];
   destructInPlace(&mArray[mElementCount - 1]);
   mElementCount--;
}

template<class T> inline void Vector<T>::erase_fast(U32 index)
{
   // CAUTION: this operator does NOT maintain list order
   // Copy the last element into the deleted 'hole' and decrement the
   //   size of the vector.
   // Assert: index >= 0 && index < mElementCount

   if(index != mElementCount - 1)
      mArray[index] = mArray[mElementCount - 1];
   destructInPlace(&mArray[mElementCount - 1]);
   mElementCount--;
}

template<class T> inline T& Vector<T>::first()
{
   return mArray[0];
}

template<class T> inline const T& Vector<T>::first() const
{
   return mArray[0];
}

template<class T> inline T& Vector<T>::last()
{
   TNLAssert(mElementCount != 0, "Error, no last element of a zero sized array!");
   return mArray[mElementCount - 1];
}

template<class T> inline const T& Vector<T>::last() const
{
   return mArray[mElementCount - 1];
}

template<class T> inline void Vector<T>::clear()
{
   setSize(0);
}

template<class T> inline void Vector<T>::deleteAndClear()
{
   for(U32 i = 0; i < mElementCount; i++)
      delete mArray[i];

   clear();
}

//-----------------------------------------------------------------------------

template<class T> inline Vector<T>& Vector<T>::operator=(const Vector<T>& p)
{
   destroy(0, mElementCount);
   mElementCount = 0;
   checkSize(p.mElementCount);
   construct(0, p.mElementCount, p.mArray);
   mElementCount = p.mElementCount;
   return *this;
}

template<class T> inline S32 Vector<T>::size() const
{
   return (S32)mElementCount;
}

template<class T> inline bool Vector<T>::empty() const
{
   return (mElementCount == 0);
}

// The following were removed by CE because they served no function and cluttered the IDE
//template<class T> inline T& Vector<T>::front()
//{
//   //return *begin();
//   return;
//}
//
//template<class T> inline const T& Vector<T>::front() const
//{
//   //return *begin();
//   return;
//}
//
//template<class T> inline T& Vector<T>::back()
//{
//   //return *end();
//   return;
//}
//
//template<class T> inline const T& Vector<T>::back() const
//{
//   //return *end();
//   return;
//}


template<class T> inline T& Vector<T>::get(S32 index)
{
   return mArray[index];
}


template<class T> inline const T& Vector<T>::get(S32 index) const
{
   return mArray[index];
}


template<class T> inline void Vector<T>::push_front(const T &x)
{
   insert(0);
   mArray[0] = x;
}

template<class T> inline void Vector<T>::push_back(const T &x)
{
   checkSize(mElementCount + 1);
   mElementCount++;
   constructInPlace(mArray + mElementCount - 1, &x);
}

template<class T> inline T& Vector<T>::pop_front()
{
   T& t = mArray[0];
   erase(U32(0));
   return t;
}

template<class T> inline T& Vector<T>::pop_back()
{
   T& t = mArray[mElementCount - 1];
   mElementCount--;
   destructInPlace(mArray + mElementCount);
   return t;
}

template<class T> inline T& Vector<T>::operator[](U32 index)
{
   return mArray[index];
}

template<class T> inline const T& Vector<T>::operator[](U32 index) const
{
   return mArray[index];
}

template<class T> inline void Vector<T>::reserve(U32 size)
{
   checkSize(size);
}

//template<class T> inline void Vector<T>::set(void * addr, U32 sz)
//{
//   setSize(sz);
//   if (addr)
//      memcpy(address(),addr,sz*sizeof(T));
//}

//-----------------------------------------------------------------------------

template<class T> inline void Vector<T>::checkSize(U32 newCount)
{
   if(newCount <= mArraySize)
      return;

   U32 blk = VectorBlockSize - (newCount % VectorBlockSize);
   newCount += blk;

   T *newArray = (T *) malloc(sizeof(T) * newCount);
   T *oldArray = mArray;

   mArray = newArray;
   construct(0, mElementCount, oldArray);
   mArray = oldArray;
   destroy(0, mElementCount);
   free(oldArray);
   mArray = newArray;
   mArraySize = newCount;
}

typedef int (QSORT_CALLBACK *qsort_compare_func)(const void *, const void *);

template<class T> inline void Vector<T>::sort(compare_func f)
{
   qsort(address(), size(), sizeof(T), (qsort_compare_func) f);
}

};

#endif //_TNL_TVECTOR_H_

