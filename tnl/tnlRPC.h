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

#ifndef _TNL_RPC_H_
#define _TNL_RPC_H_

#ifndef _TNL_NETEVENT_H_
#include "tnlNetEvent.h"
#endif

#ifndef _TNL_METHODDISPATCH_H_
#include "tnlMethodDispatch.h"
#endif

namespace TNL {

/*! @page rpcdesc RPC in the Torque Network Library

The Torque Network Library has a powerful, yet simple to use Remote
Procedure Call framework for passing information through a network
connection.  Subclasses of EventConnection and NetObject can declare
member functions using the RPC macros so that when called the function 
arguments are sent to the remote EventConnection or NetObject(s) associated
with that object.

For example, suppose you have a connection class called SimpleEventConnection:
@code
class SimpleEventConnection : public EventConnection
{
public:
   TNL_DECLARE_RPC(rpcPrintString, (StringPtr theString, U32 messageCount));
};

TNL_IMPLEMENT_RPC(SimpleEventConnection, rpcPrintString, 
   (StringPtr theString, messageCount), (theString, messageCount),
   NetClassGroupGameMask, RPCGuaranteedOrdered, RPCDirAny, 0)
{
   for(U32 i = 0; i < messageCount; i++)
      printf("%s", theString.getString());
}
 
 ...
void somefunction(SimpleEventConnection *connection)
{
   connection->rpcPrintString("Hello World!", 5);
}
@endcode

In this example the class SimpleEventConnection is declared to have
a single RPC method named rpcPrintString.  The TNL_DECLARE_RPC macro
can just be viewed as a different way of declaring a class's member functions,
with the name as the first argument and the parenthesized parameter list
as the second.  Since RPC calls execute on a remote host, they never have
a return value - although a second RPC could be declared to pass messages
in the other direction.

The body of the RPC method is declared using the TNL_IMPLEMENT_RPC macro,
which has some additional arguments: the named parameter list without the types,
which NetClassMask the RPC is valid in, what level of data guarantee it uses, 
the direction it is allowed to be called on the connection and a version number.  
The body of the function, which in this case prints the passed message "Hello, World!" 5 times
to stdout, is executed on the remote host from which the method was originally invoked.

As the somefunction code demonstrates, RPC's are invoked in the same way
as any other member function in the class.

RPCs behave like virtual functions in that their bodies can be overridden
in subclasses that want to implement new behavior for the message.  The class
declaration for an overridden RPC should include the TNL_DECLARE_RPC_OVERRIDE
macro used for each method that will be redefined.  The TNL_IMPLEMENT_RPC_OVERRIDE
macro should be used outside the declaration of the class to implement
the body of the new RPC.

Internally the RPC macros construct new NetEvent classes and encapsulate the
function call arguments using the FunctorDecl template classes.  By default
the following types are allowed as parameters to RPC methods:

 - S8, U8
 - S16, U16
 - S32, U32
 - F32
 - Int<>
 - SignedInt<>
 - Float<>
 - SignedFloat<>
 - RangedU32<>
 - bool
 - StringPtr
 - StringTableEntry
 - ByteBufferPtr
 - IPAddress
 - Vector<> of all the preceding types

New types can be supported by implementing a template override for the
Types::read and Types::write functions.  All arguments to RPCs must be
passed by value (ie no reference or pointer types).

The Int, SignedInt, Float, SignedFloat and RangedU32 template types use the
template parameter(s) to specify the number of bits necessary to transmit that
variable across the network.  For example:

@code
  ...
 TNL_DECLARE_RPC(someTestFunction, (Int<4> fourBitInt, SignedFloat<7> aFloat, 
    RangedU32<100, 199> aRangedU32);
  ...
@endcode
The preceding RPC method would use 4 + 7 + 7 = 18 bits to transmit the arguments
to the function over the network, not including the RPC event overhead.
*/

/// Enumeration for valid directions that RPC messages can travel
enum RPCDirection {
   RPCDirAny            = NetEvent::DirAny,           ///< This RPC can be sent from the server or the client
   RPCDirServerToClient = NetEvent::DirServerToClient,///< This RPC can only be sent from the server to the client
   RPCDirClientToServer = NetEvent::DirClientToServer,///< This RPC can only be sent from the client to the server
};

/// Type of data guarantee this RPC should use
enum RPCGuaranteeType {
   RPCGuaranteedOrdered = NetEvent::GuaranteedOrdered, ///< RPC event delivery is guaranteed and will be processed in the order it was sent relative to other ordered events and RPCs
   RPCGuaranteed        = NetEvent::Guaranteed,        ///< RPC event delivery is guaranteed and will be processed in the order it was received
   RPCUnguaranteed      = NetEvent::Unguaranteed,      ///< Event delivery is not guaranteed - however, the event will remain ordered relative to other unguaranteed events
   RPCGuaranteedOrderedBigData = NetEvent::GuaranteedOrderedBigData ///< Bigger data size support
};

/// Macro used to declare the implementation of an RPC method on an EventConnection subclass.
///
/// The macro should be used in place of a member function parameter declaration,
/// with the body code (to be executed on the remote side of the RPC) immediately
/// following the TNL_IMPLEMENT_RPC macro call.
#define TNL_IMPLEMENT_RPC(className, name, args, argNames, groupMask, guaranteeType, eventDirection, rpcVersion) \
class RPC_##className##_##name : public TNL::RPCEvent { \
public: \
   TNL::FunctorDecl<void (className::*) args > mFunctorDecl;\
   RPC_##className##_##name() : TNL::RPCEvent(guaranteeType, eventDirection), mFunctorDecl(&className::name##_remote) { mFunctor = &mFunctorDecl; } \
   TNL_DECLARE_CLASS( RPC_##className##_##name ); \
   bool checkClassType(TNL::Object *theObject) { return dynamic_cast<className *>(theObject) != NULL; } }; \
   TNL_IMPLEMENT_NETEVENT( RPC_##className##_##name, groupMask, rpcVersion ); \
   void className::name args { if(!canPostNetEvent()) return; RPC_##className##_##name *theEvent = new RPC_##className##_##name; theEvent->mFunctorDecl.set argNames ; postNetEvent(theEvent); } \
   TNL::NetEvent * className::name##_construct args { RPC_##className##_##name *theEvent = new RPC_##className##_##name; theEvent->mFunctorDecl.set argNames ; return theEvent; } \
   /*void className::name##_test args { RPC_##className##_##name *theEvent = new RPC_##className##_##name; theEvent->mFunctorDecl.set argNames ; TNL::PacketStream ps; theEvent->pack(this, &ps); ps.setBytePosition(0); theEvent->unpack(this, &ps); theEvent->process(this); }*/ \
   void className::name##_remote args

// those _test is not needed, removing it can reduce compile / linker memory usage

/// Base class for RPC events.
///
/// All declared RPC methods create subclasses of RPCEvent to send data across the wire
class RPCEvent : public NetEvent
{
public:
   Functor *mFunctor;
   /// Constructor call from within the rpc<i>Something</i> method generated by the TNL_IMPLEMENT_RPC macro.
   RPCEvent(RPCGuaranteeType gType, RPCDirection dir);
   void pack(EventConnection *ps, BitStream *bstream);
   void unpack(EventConnection *ps, BitStream *bstream);
   virtual bool checkClassType(Object *theObject) = 0;

   void process(EventConnection *ps);
};

/// Declares an RPC method within a class declaration, which can be used for declaring methods in a superclass that will be implemented in a subclass using TNL_DECLARE_RPC and friends.
#define TNL_DECLARE_RPC_INTERFACE(name, args) virtual void name args = 0; /*virtual void name##_test args = 0;*/ virtual TNL::NetEvent * name##_construct args = 0; virtual void name##_remote args = 0

/// Declares an RPC method within a class declaration.  Creates two method prototypes - one for the host side of the RPC call, and one for the receiver, which performs the actual method.
#define TNL_DECLARE_RPC(name, args) void name args; /*void name##_test args;*/ virtual TNL::NetEvent * name##_construct args; virtual void name##_remote args

/// Declares an override to an RPC method declared in a parent class.
#define TNL_DECLARE_RPC_OVERRIDE(name, args) void name##_remote args

/// Macro used to declare the body of an overridden RPC method.
#define TNL_IMPLEMENT_RPC_OVERRIDE(className, name, args) \
   void className::name##_remote args

/// Constructs a NetEvent that will represent the specified RPC invocation.  This
/// macro is used to construct a single RPC that can then be posted to multiple
/// connections, instead of allocating an RPCEvent for each connection.
#define TNL_RPC_CONSTRUCT_NETEVENT(object, rpcMethod, args) (object)->rpcMethod##_construct args

};

#endif
