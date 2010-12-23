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

#ifndef _TNLTHREAD_H_
#define _TNLTHREAD_H_

#include "tnl.h"
#include "tnlNetBase.h"
#include "tnlMethodDispatch.h"

#if defined (TNL_OS_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif

namespace TNL
{

/// Platform independent semaphore class.
///
/// The semaphore class wraps OS specific semaphore functionality for thread synchronization.
class Semaphore
{
#ifdef TNL_OS_WIN32
   HANDLE mSemaphore;
#else
   sem_t mSemaphore;
#endif
public:
   /// Semaphore constructor - initialCount specifies how many wait calls
   /// will be let through before an increment is required.
   Semaphore(U32 initialCount = 0, U32 maximumCount = 1024);
   ~Semaphore();

   /// Thread calling wait will block as long as the semaphore's count
   /// is zero.  If the semaphore is incremented, one of the waiting threads
   /// will be awakened and the semaphore will decrement.
   void wait();

   /// Increments the semaphore's internal count.  This will wake
   /// count threads that are waiting on this semaphore.
   void increment(U32 count = 1);
};

/// Platform independent Mutual Exclusion implementation
class Mutex
{
#ifdef TNL_OS_WIN32
   CRITICAL_SECTION mLock;
#else
   pthread_mutex_t mMutex;
#endif
public:
   /// Mutex constructor
   Mutex();
   /// Mutex destructor
   ~Mutex();

   /// Locks the Mutex.  If another thread already has this Mutex
   /// locked, this call will block until it is unlocked.  If the lock
   /// method is called from a thread that has already locked this Mutex,
   /// the call will not block and the thread will have to unlock
   /// the mutex for as many calls as were made to lock before another
   /// thread will be allowed to lock the Mutex.
   void lock();

   /// Unlocks the Mutex.  The behavior of this method is undefined if called
   /// by a thread that has not previously locked this Mutex.
   void unlock();

   /// Attempts to acquire a lock to this Mutex, without blocking.
   /// Returns true if the calling thread was able to lock the Mutex and
   /// false if the Mutex was already locked by another thread.
   bool tryLock();
};

/// Platform independent Thread class.
class Thread : public Object
{
protected:
   U32 mReturnValue; ///< Return value from thread function

#ifdef TNL_OS_WIN32
   HANDLE mThread;
#else
   pthread_t mThread;
#endif
public:
   /// run function called when thread is started.
   virtual U32 run() = 0;
   /// Thread constructor.
   Thread();
   /// Thread destructor.
   ~Thread();

   /// starts the thread's main run function.
   void start();
};

/// Platform independent per-thread storage class.
class ThreadStorage
{
#ifdef TNL_OS_WIN32
   DWORD mTlsIndex;
#else
   pthread_key_t mThreadKey;
#endif
public:
   /// ThreadStorage constructor.
   ThreadStorage();
   /// ThreadStorage destructor.
   ~ThreadStorage();

   /// returns the per-thread stored void pointer for this ThreadStorage.  The default value is NULL.
   void *get();
   /// sets the per-thread stored void pointer for this ThreadStorage object.
   void set(void *data);
};

/// Managing object for a queue of worker threads that pass
/// messages back and forth to the main thread.  ThreadQueue
/// methods declared with the TNL_DECLARE_THREADQ_METHOD macro
/// are special -- if they are called from the main thread,
/// they will be executed on one of the worker threads and vice
/// versa.
class ThreadQueue : public Object
{
   class ThreadQueueThread : public Thread
   {
      ThreadQueue *mThreadQueue;
      public:
      ThreadQueueThread(ThreadQueue *);
      U32 run();
   };
   friend class ThreadQueueThread;
   /// list of worker threads on this ThreadQueue
   Vector<Thread *> mThreads;
   /// list of calls to be processed by the worker threads
   Vector<Functor *> mThreadCalls;
   /// list of calls to be processed by the main thread
   Vector<Functor *> mResponseCalls;
   /// Synchronization variable that manages worker threads
   Semaphore mSemaphore;
   /// Internal Mutex for synchronizing access to thread call vectors.
   Mutex mLock;
   /// Storage variable that tracks whether this is the main thread or a worker thread.
   ThreadStorage mStorage;
protected:
   /// Locks the ThreadQueue for access to member variables.
   void lock() { mLock.lock(); }
   /// Unlocks the ThreadQueue.
   void unlock() { mLock.unlock(); }
   /// Posts a marshalled call onto either the worker thread call list or the response call list.
   void postCall(Functor *theCall);
   /// Dispatches the next available worker thread call.  Called internally by the worker threads when they awaken from the semaphore.
   void dispatchNextCall();
   /// helper function to determine if the currently executing thread is a worker thread or the main thread.
   bool isMainThread() { return (bool) mStorage.get(); }
   ThreadStorage &getStorage() { return mStorage; }
   /// called by each worker thread when it starts for subclass initialization of worker threads.
   virtual void threadStart() { }
public:
   /// ThreadQueue constructor.  threadCount specifies the number of worker threads that will be created.
   ThreadQueue(U32 threadCount);
   ~ThreadQueue();

   /// Dispatches all ThreadQueue calls queued by worker threads.  This should
   /// be called periodically from a main loop.
   void dispatchResponseCalls();
};

/// Declares a ThreadQueue method on a subclass of ThreadQueue.
#define TNL_DECLARE_THREADQ_METHOD(func, args) \
   void func args; \
   void func##_body args

/// Declares the implementation of a ThreadQueue method.
#define TNL_IMPLEMENT_THREADQ_METHOD(className, func, args, argNames) \
   void className::func args { \
   FunctorDecl<void (className::*)args> *theCall = new FunctorDecl<void (className::*)args>(&className::func##_body); \
   theCall->set argNames; \
   postCall(theCall); \
   }\
   void className::func##_body args


};

#endif

