//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// A simple, but fast stack structure

#ifndef _STACK_H_
#define _STACK_H_

#include "tnlAssert.h"
#include <cstddef> // For size_t

namespace Zap
{

template<typename T, std::size_t capacity>
class Stack
{
private:
   T mItems[capacity];
   std::size_t mCount;

public:
   Stack() : mCount(0) {}

   const T &top() const
   {
      TNLAssert(mCount > 0, "Cannot get top, stack is empty!");
      return mItems[mCount - 1];
   }

   T &top()
   {
      TNLAssert(mCount > 0, "Cannot get top, stack is empty!");
      return mItems[mCount - 1];
   }

   void push(const T &item)
   {
      TNLAssert(mCount < capacity, "Cannot push item, stack is full!");
      mItems[mCount] = item;
      ++mCount;
   }

   void pop()
   {
      TNLAssert(mCount > 0, "Cannot pop, stack is already empty!");
      --mCount;
   }
};

}

#endif // _STACK_H_ 
