//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Renderer.h"

namespace Zap
{

std::unique_ptr<Renderer> Renderer::mInstance;

// Static
void Renderer::setInstance(std::unique_ptr<Renderer>&& instance)
{
   TNLAssert(mInstance == nullptr, "Cannot set renderer instance; instance already exists!");
   mInstance = std::move(instance);
}

// Static
Renderer& Renderer::get()
{
   TNLAssert(mInstance != nullptr, "You must create a Renderer before using get()!");
   return *mInstance;
}

}