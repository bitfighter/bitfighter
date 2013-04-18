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

#ifndef _TNL_NETBASE_H_
#define _TNL_NETBASE_H_

//------------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Disable POSIX deprecation, certain security warnings that seem to be specific to VC++
#endif

#ifndef _TNL_TYPES_H_
#include "tnlTypes.h"
#endif

#ifndef _TNL_PLATFORM_H_
#include "tnlPlatform.h"
#endif

#ifndef _TNL_ASSERT_H_
#include "tnlAssert.h"
#endif

#ifndef _TNL_BITSET_H_
#include "tnlBitSet.h"
#endif

#ifndef _TNL_VECTOR_H_
#include "tnlVector.h"
#endif

namespace TNL
{

/// NetClassTypes are used to define the ranges of individual
/// class identifiers, in order to reduce the number of bits
/// necessary to identify the class of an object across the
/// network.
///
/// For example, if there are only 21 classes declared
/// of NetClassTypeObject, the class identifier only needs to
/// be sent using 5 bits.
enum NetClassType {
   NetClassTypeNone = -1,  ///< Not an id'able network class
   NetClassTypeObject = 0, ///< Game object classes
   NetClassTypeDataBlock,  ///< Data block classes
   NetClassTypeEvent,      ///< Event classes
   NetClassTypeCount,
};
// >>>>> Note NetClassTypeNames def in netBase!


/// NetClassGroups are used to define different service types
/// for an application.
///
/// Each network-related class can be marked as valid across one or
/// more NetClassGroups.  Each network connection belongs to a
/// particular group, and can only transmit objects that are valid
/// in that group.
enum NetClassGroup {
   NetClassGroupGame,      ///< Group for game related network classes
   NetClassGroupCommunity, ///< Group for community server/authentication classes   >>> unused in Bitfighter
   NetClassGroupMaster,    ///< Group for simple master server.
   NetClassGroupUnused2,   ///< Reserved group.                                     >>> unused in Bitfighter
   NetClassGroupCount,
   NetClassGroupInvalid = NetClassGroupCount,
};
// >>>>> Note NetClassGroupNames def in netBase!


/// Mask values used to indicate which NetClassGroup(s) a NetObject or NetEvent
/// can be transmitted through.
enum NetClassMask {
   NetClassGroupGameMask      = 1 << NetClassGroupGame,
   NetClassGroupCommunityMask = 1 << NetClassGroupCommunity,      // Unused in Bitfighter
   NetClassGroupMasterMask    = 1 << NetClassGroupMaster,

   NetClassGroupAllMask = (1 << NetClassGroupCount) - 1,
};


class Object;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/// NetClassRep class instances represent each declared NetClass
/// and are used to construct object instances.
///
/// Core functionality for TNL-registered class manipulation.
///
/// @section NetClassRep_intro Introduction (or, Why AbstractClassRep?)
///
/// TNL requires the ability to programatically instantiate classes, by name or by
/// ID numbers. This is used for handling connections, ghosting, working with
/// events, and in many other areas. Torque uses this functionality for its scripting
/// language, too.
///
/// Since standard C++ doesn't provide a function to create a new instance of
/// an arbitrary class at runtime, one must be created. This is what
/// NetClassRep and NetClassRepInstance are all about. They allow the registration
/// and instantiation of arbitrary classes at runtime.
///
/// @see TNL::Object
///
/// @note In general, you will only access the functionality implemented in this class via
///       TNL::Object::create(). Most of the time, you will only ever need to use this part
///       part of the engine indirectly - ie, you will use the networking system.
///       <b>The following discussion is really only relevant for advanced TNL users.</b>
///
/// @section NetClassRep_netstuff NetClasses and Class IDs
///
/// TNL supports a notion of group, type, and direction for objects passed over
/// the network. Class IDs are assigned sequentially per-group, per-type, so that, for instance,
/// the IDs assigned to TNL::NetObjects are seperate from the IDs assigned to NetEvents.
/// This can translate into significant bandwidth savings (especially since the size of the fields
/// for transmitting these bits are determined at run-time based on the number of IDs given out.
///
/// @section NetClassRep_details NetClassRep Internals
///
/// NCR does some preparatory work at runtime before execution is passed to main(), through static
/// initialization. In actual fact, this preparatory work is done by the NetClassRepInstasnce
/// template. Let's examine this more closely.
///
/// If we examine TNL::Object, we see that two macros must be used in the definition of a
/// properly integrated objects. Let's look at an example:
///
/// @code
///      // This is from inside the class definition...
///      DECLARE_CONOBJECT(TestObject);
///
/// // And this is from outside the class definition...
/// IMPLEMENT_CONOBJECT(TestObject);
/// @endcode
///
/// What do these things actually do?
///
/// Not all that much, in fact. They expand to code something like this:
///
/// @code
///      // This is from inside the class definition...
///      static NetClassRepInstance<TestObject> dynClassRep;
///      static NetClassRep* getParentStaticClassRep();
///      static NetClassRep* getStaticClassRep();
///      virtual NetClassRep* getClassRep() const;
/// @endcode
///
/// @code
/// // And this is from outside the class definition...
/// NetClassRep* TestObject::getClassRep() const { return &TestObject::dynClassRep; }
/// NetClassRep* TestObject::getStaticClassRep() { return &dynClassRep; }
/// NetClassRep* TestObject::getParentStaticClassRep() { return Parent::getStaticClassRep(); }
/// NetClassRepInstance<TestObject> TestObject::dynClassRep("TestObject", 0, -1, 0);
/// @endcode
///
/// As you can see, getClassRep(), getStaticClassRep(), and getParentStaticClassRep() are just
/// accessors to allow access to various NetClassRepInstance instances. This is where the Parent
/// typedef comes into play as well - it lets getParentStaticClassRep() get the right
/// class rep.
///
/// In addition, dynClassRep is declared as a member of TestObject, and defined later
/// on. Much like Torque's ConsoleConstructor, NetClassRepInstances add themselves to a global linked
/// list in their constructor.
///
/// Then, when NetClassRep::initialize() is called, we iterate through
/// the list and perform the following tasks:
///      - Assigns network IDs for classes based on their NetGroup membership. Determines
///        bit allocations for network ID fields.
///
/// @nosubgrouping
class NetClassRep
{
   friend class Object;

protected:
   NetClassRep();
   virtual ~NetClassRep() {}
   U32 mClassGroupMask;                ///< Mask for which class groups this class belongs to.
   S32 mClassVersion;                  ///< The version number for this class.
   NetClassType mClassType;            ///< Which class type is this?
   U32 mClassId[NetClassGroupCount];   ///< The id for this class in each class group.
   char *mClassName;                   ///< The unmangled name of the class.

   U32 mInitialUpdateBitsUsed; ///< Number of bits used on initial updates of objects of this class.
   U32 mPartialUpdateBitsUsed; ///< Number of bits used on partial updates of objects of this class.
   U32 mInitialUpdateCount;    ///< Number of objects of this class constructed over a connection.
   U32 mPartialUpdateCount;    ///< Number of objects of this class updated over a connection.

   /// Next declared NetClassRep.
   ///
   /// These are stored in a linked list built by the macro constructs.
   NetClassRep *mNextClass;

   static NetClassRep *mClassLinkList;                                      ///< Head of the linked class list.
   static Vector<NetClassRep *> mClassTable[NetClassGroupCount][NetClassTypeCount]; ///< Table of NetClassReps for construction by class ID.
   static U32 mClassCRC[NetClassGroupCount];                                ///< Internally computed class group CRC.
   static bool mInitialized;                                                ///< Set once the class tables are built, from initialize.

   /// mNetClassBitSize is the number of bits needed to transmit the class ID for a group and type.
   static U32 mNetClassBitSize[NetClassGroupCount][NetClassTypeCount];

   /// @name Object Creation
   ///
   /// These helper functions let you create an instance of a class by name or ID.
   ///
   /// @note Call Object::create() instead of these.
   /// @{

   static Object *create(const char *className);
   static Object *create(const U32 groupId, const U32 typeId, const U32 classId);

   /// @}

public:
   U32 getClassId(NetClassGroup classGroup) const; ///< Returns the class ID, within its type, for the particular group.
   NetClassType getClassType() const;              ///< Returns the NetClassType of this class.
   S32 getClassVersion() const;                    ///< Returns the version of this class.
   const char *getClassName() const;               ///< Returns the string class name.

   /// Records bits used in the initial update of objects of this class.
   void addInitialUpdate(U32 bitCount)
   {
      mInitialUpdateCount++;
      mInitialUpdateBitsUsed += bitCount;
   }

   /// Records bits used in a partial update of an object of this class.
   void addPartialUpdate(U32 bitCount)
   {
      mPartialUpdateCount++;
      mPartialUpdateBitsUsed += bitCount;
   }

   virtual Object *create() const = 0;             ///< Creates an instance of the class this represents.

   /// Returns the number of classes registered under classGroup and classType.
   static U32 getNetClassCount(U32 classGroup, U32 classType)
      { return mClassTable[classGroup][classType].size(); }

   /// Returns the number of bits necessary to transmit class ids of the specified classGroup and classType.
   static U32 getNetClassBitSize(U32 classGroup, U32 classType)
      { return mNetClassBitSize[classGroup][classType]; }

   /// Returns true if the given class count is on a version boundary
   //  i.e. if the next RPC is a higher version than the current one, indicated by count
   static bool isVersionBorderCount(U32 classGroup, U32 classType, U32 count)
      { return count == U32(mClassTable[classGroup][classType].size()) ||
               (count > 0 && mClassTable[classGroup][classType][count]->getClassVersion() !=
                        mClassTable[classGroup][classType][count - 1]->getClassVersion()); }

   static NetClassRep *getClass(U32 classGroup, U32 classType, U32 index)
      { return mClassTable[classGroup][classType][index]; }
      
   /// Returns a CRC of class data, for checking on connection.
   static U32 getClassGroupCRC(NetClassGroup classGroup);

   /// Initializes the class table and associated data - called from TNL::init().
   static void initialize();

   /// Logs the bit usage information of all the NetClassReps
   static void logBitUsage();
};

inline U32 NetClassRep::getClassId(NetClassGroup classGroup) const
{
   return mClassId[classGroup];
}

inline NetClassType NetClassRep::getClassType() const
{
   return mClassType;
}

inline S32 NetClassRep::getClassVersion() const
{
   return mClassVersion;
}

inline const char * NetClassRep::getClassName() const
{
   return mClassName;
}

inline U32 NetClassRep::getClassGroupCRC(NetClassGroup classGroup)
{
   return mClassCRC[classGroup];
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/// NetClassRepInstance - one of these templates is instantiated for each
/// class that is declared via the IMPLEMENT_* macros below.
///
/// There will be an instance for each networkable class.
template <class T>
class NetClassRepInstance : public NetClassRep
{
public:
   /// Each class registers itself through the constructor of its NetClassInstance
   NetClassRepInstance(const char *className, U32 groupMask, NetClassType classType, S32 classVersion)
   {
      // Store data about ourselves
      mClassName      = strdup(className);
      mClassType      = classType;
      mClassGroupMask = groupMask;
      mClassVersion   = classVersion;
      for(U32 i = 0; i < NetClassGroupCount; i++)
         mClassId[i] = 0;

      // link the class into our global list
      mNextClass = mClassLinkList;
      mClassLinkList = this;
   }
   ~NetClassRepInstance()
   {
      free(mClassName);
   }

   /// Each NetClassRepInstance overrides the virtual create() function to construct its object instances.
   Object *create() const
   {
      return new T;
   }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

class RefPtrData
{
   U32 mRefCount;                  ///< Reference counter for RefPtr objects.
   friend class RefObjectRef;

public:
   RefPtrData() {mRefCount = 0;}
   RefPtrData(const RefPtrData &copy) {mRefCount = 0;}
   virtual ~RefPtrData();

   /// Object destroy self call (from RefPtr).
   ///
   /// @note Override if this class has specially allocated memory.
   virtual void destroySelf();

   void incRef()
   {
      mRefCount++;
   }

   void decRef()
   {
      mRefCount--;
      if(!mRefCount)
         destroySelf();
   }
};


class SafeObjectRef;

class SafePtrData
{
   SafeObjectRef *mFirstObjectRef; ///< The head of the linked list of safe object references.
   friend class SafeObjectRef;
public:
   SafePtrData() {mFirstObjectRef = NULL;}
   SafePtrData(const SafePtrData &copy) {mFirstObjectRef = NULL;}
   virtual ~SafePtrData();
};


//------------------------------------------------------------------------------

/// Base class for all NetObject, NetEvent, NetConnection and NetInterface instances.
///
/// @section TNLObject_references Ways of Referencing Object
///
/// Object supports both reference counting and safe pointers.  Ref counted objects
/// are accessed via the RefPtr template class.  Once an object has been assigned using
/// a RefPtr, it will be deleted when all the reference pointers to it are destructed.
/// Object instances can also be safely referenced with the SafePtr template.  The
/// SafePtr template instances are automatically set NULL when the object they point
/// to is deleted.
///
/// @section TNLObject_basics The Basics
///
/// Any object which you want to work with the networking system should derive from this,
/// and access functionality through the static interface.
///
/// This class is always used with the TNL_DECLARE_CLASS and TNL_IMPLEMENT_CLASS macros.
///
/// @code
/// // A very basic example object. It will do nothing!
/// class TestObject : public NetObject {
///      // Must provide a Parent typedef so the console system knows what we inherit from.
///      typedef NetObject Parent;
///
///      // This does a lot of menial declaration for you.
///      TNL_DECLARE_CLASS(TestObject);
/// }
/// @endcode
///
/// @code
/// // And the accordant implementation...
/// TNL_IMPLEMENT_CLASS(TestObject);
///
/// @endcode
///
/// That's all you need to do to get a class registered with the TNL object system. At this point,
/// you can instantiate it via TNL::Object::create, ghost it, and so forth.
///
/// @see NetClassRepInstance for gory implementation details.
/// @nosubgrouping

class Object : public RefPtrData, public SafePtrData
{
   friend class RefObjectRef;
public:
   /// Returns the NetClassRep associated with this object.
   virtual NetClassRep* getClassRep() const;

   /// Get our class ID within the specified NetClassGroup.
   U32 getClassId(NetClassGroup classGroup) const;

   /// Get our unmangled class name.
   const char *getClassName() const;

   /// @name Object Creation
   ///
   /// These helper functions let you create an instance of a class by name or ID.
   ///
   /// @note Call these instead of NetClassRep::create
   /// @{

   /// static function to create an instance of a named class
   static Object* create(const char* className) { return NetClassRep::create(className); }

   /// static function to create an instance of a class identified by a class group, class type and class id
   static Object* create( const NetClassGroup groupId,
                          const NetClassType typeId,
                          const U32 classId)
   {
      return NetClassRep::create(groupId, typeId, classId);
   }

   /// @}
};

inline U32 Object::getClassId(NetClassGroup classGroup) const
{
   TNLAssert(getClassRep() != NULL,
               "Cannot get class id from non-declared dynamic class");
   return getClassRep()->getClassId(classGroup);
}

inline const char * Object::getClassName() const
{
   TNLAssert(getClassRep() != NULL,
               "Cannot get class name from non-declared dynamic class");
   return getClassRep()->getClassName();
}

/// Base class for Object reference counting
class RefObjectRef
{
protected:
   RefPtrData *mObject; ///< The object this RefObjectRef references.

public:

   /// Constructor, assigns from the object and increments its reference count if it's not NULL.
   RefObjectRef(RefPtrData *object = NULL)
   {
      mObject = object;
      if(object)
         object->incRef();
   }

   /// Destructor, dereferences the object, if there is one.
   ~RefObjectRef()
   {
      if(mObject)
         mObject->decRef();
   }

   /// Assigns this reference object from an existing Object instance.
   void set(RefPtrData *object)
   {
      if(object)
         object->incRef(); // increment first to avoid delete problem when (object == mObject)
      if(mObject)
         mObject->decRef();
      mObject = object;
   }
};

/// Reference counted object template pointer class.
///
/// Instances of this template class can be used as pointers to
/// instances of Object and its subclasses.  The object will not
/// be deleted until all of the RefPtr instances pointing to it
/// have been destructed.
template <class T> class RefPtr : public RefObjectRef
{
public:
   RefPtr() : RefObjectRef() {}
   RefPtr(T *ptr) : RefObjectRef(ptr) {}
   RefPtr(const RefPtr<T>& ref) : RefObjectRef((RefPtrData *) ref.mObject) {}

   RefPtr<T>& operator=(const RefPtr<T>& ref)
   {
      set((Object *) ref.mObject);
      return *this;
   }
   RefPtr<T>& operator=(T *ptr)
   {
      set(ptr);
      return *this;
   }
   bool isNull() const   { return mObject == 0; }
   bool isValid() const  { return mObject != 0; }
   T* operator->() const { return static_cast<T*>(mObject); }
   T& operator*() const  { return *static_cast<T*>(mObject); }
   operator T*() const   { return static_cast<T*>(mObject); }
   operator T*() { return static_cast<T*>(mObject); }
   T* getPointer() const { return static_cast<T*>(mObject); }
};

/// Base class for Object safe pointers.
class SafeObjectRef
{
friend class SafePtrData;
protected:
   SafePtrData *mObject;          ///< The object this is a safe pointer to, or NULL if the object has been deleted.
   SafeObjectRef *mPrevObjectRef; ///< The previous SafeObjectRef for mObject.
   SafeObjectRef *mNextObjectRef; ///< The next SafeObjectRef for mObject.
public:
   SafeObjectRef(SafePtrData *object);
   SafeObjectRef();
   void set(SafePtrData *object);
   ~SafeObjectRef();

   void registerReference();   ///< Links this SafeObjectRef into the doubly linked list of SafeObjectRef instances for mObject.
   void unregisterReference(); ///< Unlinks this SafeObjectRef from the doubly linked list of SafeObjectRef instance for mObject.
};

inline void SafeObjectRef::unregisterReference()
{
   if(mObject)
   {
      if(mPrevObjectRef)
         mPrevObjectRef->mNextObjectRef = mNextObjectRef;
      else
         mObject->mFirstObjectRef = mNextObjectRef;
      if(mNextObjectRef)
         mNextObjectRef->mPrevObjectRef = mPrevObjectRef;
   }
}

inline void SafeObjectRef::registerReference()
{
   if(mObject)
   {
      mNextObjectRef = mObject->mFirstObjectRef;
      if(mNextObjectRef)
         mNextObjectRef->mPrevObjectRef = this;
      mPrevObjectRef = NULL;
      mObject->mFirstObjectRef = this;
   }
}

inline void SafeObjectRef::set(SafePtrData *object)
{
   unregisterReference();
   mObject = object;
   registerReference();
}

inline SafeObjectRef::~SafeObjectRef()
{
   unregisterReference();
}

inline SafeObjectRef::SafeObjectRef(SafePtrData *object)
{
   mObject = object;
   registerReference();
}

inline SafeObjectRef::SafeObjectRef()
{
   mObject = NULL;
}

/// Safe object template pointer class.
///
/// Instances of this template class can be used as pointers to
/// instances of Object and its subclasses.
///
/// When the object referenced by a SafePtr instance is deleted,
/// the pointer to the object is set to NULL in the SafePtr instance.
template <class T> class SafePtr : public SafeObjectRef
{
public:
   SafePtr() : SafeObjectRef() {}
   SafePtr(T *ptr) : SafeObjectRef(ptr) {}
   SafePtr(const SafePtr<T>& ref) : SafeObjectRef((SafePtrData *) ref.mObject) {}

   SafePtr<T>& operator=(const SafePtr<T>& ref)
   {
      set((Object *) ref.mObject);
      return *this;
   }
   SafePtr<T>& operator=(T *ptr)
   {
      set(ptr);
      return *this;
   }
   bool isNull() const   { return mObject == NULL; }
   bool isValid() const  { return mObject != NULL; }
   T* operator->() const { return static_cast<T*>(mObject); }
   T& operator*() const  { return *static_cast<T*>(mObject); }
   operator T*() const   { return static_cast<T*>(mObject); }
   operator T*() { return static_cast<T*>(mObject); }
   T* getPointer() { return static_cast<T*>(mObject); }
};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// Macros for declaring and implementing Object subclasses
// the TNL_DECLARE_CLASS(className) macro needs to be placed
// in the class declaraction for each network aware class.
//
// Different TNL_IMPLEMENT_* macros can be used depending on
// which NetClassType the class belongs to.

/// The TNL_DECLARE_CLASS macro should be called within the declaration of any network class
#define TNL_DECLARE_CLASS(className) \
   static TNL::NetClassRepInstance<className> dynClassRep;      \
   virtual TNL::NetClassRep* getClassRep() const

/// The TNL_IMPLEMENT_CLASS macro should be used for classes that will be auto-constructed
/// by name only.
#define TNL_IMPLEMENT_CLASS(className) \
   TNL::NetClassRep* className::getClassRep() const { return &className::dynClassRep; } \
   TNL::NetClassRepInstance<className> className::dynClassRep(#className, 0, TNL::NetClassTypeNone, 0)


};
#endif

