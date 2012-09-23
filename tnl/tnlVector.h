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
#include <vector>


#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#ifndef _TNL_PLATFORM_H_
#include "tnlPlatform.h"
#endif

#ifndef _TNL_ASSERT_H_
#include "tnlAssert.h"
#endif


namespace TNL {
// =============================================================================

/// A dynamic array template class.
///
/// The vector grows as you insert or append
/// elements.  Insertion is fastest at the end of the array.  Resizing
/// of the array can be avoided by pre-allocating space using the
/// reserve() method.
///
/// This is now just a wrapper for stl::vector
template<class T> class VectorBase
{
protected:
   std::vector<T> innerVector;
};

// Avoid std::vector<bool>, over 10 times slower on 8 bit per byte, and need to get adderss of bool arrays.

template<> class VectorBase<bool>
{
protected:
   std::vector<char> innerVector;  // sizeof(char) must equal sizeof(bool)
};

template<class T> class Vector : public VectorBase<T>
{

public:
   Vector(const U32 initialSize = 0);
   Vector(const Vector& p);
   Vector(const std::vector<T>& p);
   Vector(const T *array, U32 length);
   ~Vector();

   Vector<T>& operator=(const Vector<T>& p);

   S32 size() const;
   bool empty() const;

   T&       get(S32 index);
   const T& get(S32 index) const;

   void push_front(const T&);
   void push_back(const T&);
   T& pop_front();
   T& pop_back();

   T& operator[](U32 index);
   const T& operator[](U32 index) const;
   T& operator[](S32 index);
   const T& operator[](S32 index) const;

   void reserve(U32 size);
   void resize(U32 size);
   void insert(U32 index);
   void insert(U32 index, const T&);
   void erase(U32 index);
   void deleteAndErase(U32 index);
   void erase_fast(U32 index);
   void deleteAndErase_fast(U32 index);
   void clear();
   void deleteAndClear();
   bool contains(const T&);
   S32 getIndex(const T&);


   T& first();
   T& last();
   const T& first() const;
   const T& last() const;

   std::vector<T>& getStlVector();
   T*   address();
   const T*   address() const;
   void reverse();


   typedef S32 (QSORT_CALLBACK *compare_func)(T *a, T *b);
   void sort(compare_func f);
};

// Note that tnlVector reserves the space whereas std::vector actually sets the size
template<class T> inline Vector<T>::Vector(const U32 initialSize)   // Constructor
{
   innerVector.reserve(initialSize);
}

template<class T> inline Vector<T>::Vector(const Vector& p)        // Copy constructor
{
   innerVector = std::vector<T>(p.innerVector);
}

template<class T> inline Vector<T>::Vector(const std::vector<T>& p)        // Constructor to wrap std::vector
{
   innerVector = p;
}

template<class T> inline Vector<T>::Vector(const T *array, U32 length)        // Constructor to wrap a C-style array
{
   innerVector = std::vector<T>(array, array + length);
}

template<class T> inline Vector<T>::~Vector() {}       // Destructor

// returns a modifiable reference to the internal std::vector object
//template<class T> inline std::vector<T>& Vector<T>::getStlVector()
//{
//   return innerVector;
//}

template<class T> inline T* Vector<T>::address()
{
   TNLAssert(sizeof(T) == sizeof(*innerVector.begin()), "sizeof(char) must equal sizeof(bool)");
   if (innerVector.begin() == innerVector.end())
      return NULL;

   return (T*)&(*innerVector.begin());
}

template<class T> const inline T* Vector<T>::address() const
{
   TNLAssert(sizeof(T) == sizeof(*innerVector.begin()), "sizeof(char) must equal sizeof(bool)");
   if (innerVector.begin() == innerVector.end())
      return NULL;

   return (T*)&(*innerVector.begin());
}

// was U32     
template<class T> inline void Vector<T>::resize(U32 size)
{
   innerVector.resize(size);
}

// inserts an empty element at the specified index
template<class T> inline void Vector<T>::insert(U32 index)
{
   TNLAssert(index <= innerVector.size(), "index out of range");
   innerVector.insert(innerVector.begin() + index, 1, T());
}

// inserts an object at a specified index
template<class T> inline void Vector<T>::insert(U32 index, const T &x)
{
   TNLAssert(index <= innerVector.size(), "index out of range");
   innerVector.insert(innerVector.begin() + index, x);
}

template<class T> inline void Vector<T>::erase(U32 index)
{
   TNLAssert(index < innerVector.size(), "index out of range");
   innerVector.erase(innerVector.begin() + index);
}


template<class T> inline void Vector<T>::deleteAndErase(U32 index)
{
   TNLAssert(index < innerVector.size(), "index out of range");
   delete innerVector[index];
   erase(index);
}


template<class T> inline void Vector<T>::erase_fast(U32 index)
{
   TNLAssert(index < innerVector.size(), "index out of range");
   // CAUTION: this operator does NOT maintain list order
   // Copy the last element into the deleted 'hole' and decrement the
   //   size of the vector.

   if(index != innerVector.size() - 1)
      std::swap(innerVector[index], innerVector[innerVector.size() - 1]);
   innerVector.pop_back();
}


template<class T> inline void Vector<T>::deleteAndErase_fast(U32 index)
{
   TNLAssert(index < innerVector.size(), "index out of range");
   // CAUTION: this operator does NOT maintain list order
   // Copy the last element into the deleted 'hole' and decrement the
   //   size of the vector.
   delete innerVector[index];
   erase_fast(index);
}


template<class T> inline T& Vector<T>::first()
{
   TNLAssert(innerVector.size() != 0, "Vector is empty");
   return *innerVector.begin();
}

template<class T> inline const T& Vector<T>::first() const
{
   TNLAssert(innerVector.size() != 0, "Vector is empty");
   return *innerVector.begin();
}

template<class T> inline T& Vector<T>::last()
{
   TNLAssert(innerVector.size() != 0, "Vector is empty");
   return *(innerVector.end() - 1);
}

template<class T> inline const T& Vector<T>::last() const
{
   TNLAssert(innerVector.size() != 0, "Vector is empty");
   return *(innerVector.end() - 1);
}

template<class T> inline void Vector<T>::clear()
{
   innerVector.clear();
}

template<class T> inline void Vector<T>::deleteAndClear()
{
   for(U32 i = 0; i < innerVector.size(); i++)
      delete innerVector[i];

   innerVector.clear();
}

template<class T> inline bool Vector<T>::contains(const T &object)
{
   return getIndex(object) != -1;
}

template<class T> inline S32 Vector<T>::getIndex(const T &object)
{
   for(U32 i = 0; i < innerVector.size(); i++)
      if(object == innerVector[i])
         return i;

   return -1;
}

// Assigns a copy of vector x as the new content for the vector object.
// The elements contained in the vector object before the call are dropped, and replaced by copies of those in vector p, if any.
// After a call to this member function, both the vector object and vector p will have the same size and compare equal to each other.
template<class T> inline Vector<T>& Vector<T>::operator=(const Vector<T>& p)
{
   innerVector = p.innerVector;
   return *this;
}

template<class T> inline S32 Vector<T>::size() const
{
   return (S32)innerVector.size();
}

template<class T> inline bool Vector<T>::empty() const
{
   return innerVector.begin() == innerVector.end();
}

template<class T> inline T& Vector<T>::get(S32 index)
{
   TNLAssert(U32(index) < innerVector.size(), "index out of range");
   return (T&)innerVector[index];
}

template<class T> inline const T& Vector<T>::get(S32 index) const
{
   TNLAssert(U32(index) < innerVector.size(), "index out of range");
   return (T&)innerVector[index];
}

template<class T> inline void Vector<T>::push_front(const T &x)
{
   insert(0);
   (T&)innerVector[0] = x;
}

template<class T> inline void Vector<T>::push_back(const T &x)
{
   innerVector.push_back(x);
}

template<class T> inline T& Vector<T>::pop_front()
{
   TNLAssert(innerVector.size() != 0, "Vector is empty");
   T& t = (T&)innerVector[0];
   innerVector.erase(innerVector.begin());
   return t;
}

template<class T> inline T& Vector<T>::pop_back()
{
   TNLAssert(innerVector.size() != 0, "Vector is empty");
   T& t = (T&)*(innerVector.end() - 1);
   innerVector.pop_back();
   return t;
}

template<class T> inline T& Vector<T>::operator[](U32 index)
{
   TNLAssert(index < innerVector.size(), "index out of range");
   return (T&)innerVector[index];
}

template<class T> inline const T& Vector<T>::operator[](U32 index) const
{
   TNLAssert(index < innerVector.size(), "index out of range");
   return (T&)innerVector[index];
}

template<class T> inline T& Vector<T>::operator[](S32 index)
{
   TNLAssert(U32(index) < innerVector.size(), "index out of range");
   return (T&)innerVector[(U32)index];
}

template<class T> inline const T& Vector<T>::operator[](S32 index) const
{
   TNLAssert(U32(index) < innerVector.size(), "index out of range");
   return (T&)innerVector[(U32)index];
}

template<class T> inline void Vector<T>::reserve(U32 size)
{
   innerVector.reserve(size);
}

// Reverses this Vector's elements in place.
template<class T> inline void Vector<T>::reverse()
{
   for(S32 i = (S32(innerVector.size()) >> 1) - 1; i >= 0; i--)
   {
      T temp = innerVector[innerVector.size() - i - 1];
      innerVector[innerVector.size() - i - 1] = innerVector[i];
      innerVector[i] = temp;
   }
}

typedef int (QSORT_CALLBACK *qsort_compare_func)(const void *, const void *);

template<class T> inline void Vector<T>::sort(compare_func f)
{
   qsort(address(), size(), sizeof(T), (qsort_compare_func) f);
}


};

#endif
