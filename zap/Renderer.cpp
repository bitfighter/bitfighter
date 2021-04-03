//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "Renderer.h"
#include "Color.h"
#include "Point.h"
#include "tnlVector.h"

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

void Renderer::setColor(F32 c, F32 alpha)
{
   setColor(c, c, c, alpha);
}

void Renderer::setColor(const Color& c, F32 alpha)
{
   setColor(c.r, c.g, c.b, alpha);
}

void Renderer::translate(const Point& offset)
{
   translate(offset.x, offset.y, 0);
}

void Renderer::rotate(F32 angle)
{
   rotate(angle, 0.0f, 0.0f, 1.0f);
}

void Renderer::scale(F32 factor)
{
   scale(factor, factor, factor);
}

void Renderer::scale(const Point& factor)
{
   scale(factor.x, factor.y, 0);
}

void Renderer::renderPointVector(const Vector<Point>* points, RenderType type)
{
   const F32* verts = reinterpret_cast<const F32*>(points->address());
   renderVertexArray(verts, points->size(), type);
}

void Renderer::renderPointVector(const Vector<Point>* points, const Point& offset, RenderType type)
{
   pushMatrix();
   translate(offset);
   renderPointVector(points, type);
   popMatrix();
}

}