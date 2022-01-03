//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

// Singleton class for Rendering APIS. I wish this wasn't a Singleton, but the lack of OOP
// in some areas of Bitfighter has forced my hand!

#ifndef _RENDERER_H_
#define _RENDERER_H_

#include "tnlTypes.h"
#include "tnlAssert.h"

#include <memory>

#ifdef TNL_OS_WIN32
   // For ARRAYSIZE, I do not understand why everyone relies on this macro, but
   // eveybody seems to assume it is defined when including the Renderer.
#  include <windows.h>
#endif

namespace TNL {
   template<class T> class Vector;
};

using namespace TNL;

namespace Zap
{

class Color;
class Point;

enum class MatrixType
{
   ModelView,
   Projection
};

enum class RenderType
{
   Points,
   Lines,
   LineStrip,
   LineLoop,
   Triangles,
   TriangleStrip,
   TriangleFan
};

enum class TextureFormat
{
    RGB,
    RGBA,
    Alpha
};

enum class DataType
{
    UnsignedByte,
    Byte,
    UnsignedShort,
    Short,
    UnsignedInt,
    Int,
    Float
};

class Renderer
{
private:
   static std::unique_ptr<Renderer> mInstance;

   // Make these inaccessible:
   Renderer(const Renderer&) = default;
   Renderer& operator=(const Renderer&) = default;

protected:
   static void setInstance(std::unique_ptr<Renderer> &&instance);
   Renderer() = default; // Constructor is only accessible to child classes.
   void initRenderer();  // Call this in child constructor!

public:
   virtual ~Renderer() = default;
   static Renderer& get();

   void setColor(F32 c, F32 alpha = 1.0f);
   void setColor(const Color& c, F32 alpha = 1.0f);

   void translate(const Point& offset);
   void rotate(F32 degAngle);
   void scale(F32 factor);
   void scale(const Point& factor);

   void renderPointVector(const Vector<Point>* points, RenderType type);
   void renderPointVector(const Vector<Point>* points, const Point& offset, RenderType type);

   // Implemented by concrete renderers //
   virtual void clear() = 0;
   virtual void clearStencil() = 0;
   virtual void clearDepth() = 0;
   virtual void setClearColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) = 0;
   virtual void setColor(F32 r, F32 g, F32 b, F32 alpha = 1.0f) = 0;
   
   virtual void setLineWidth(F32 width) = 0;
   virtual void setPointSize(F32 size) = 0;
   virtual void enableAntialiasing() = 0;
   virtual void disableAntialiasing() = 0;
   virtual void enableBlending() = 0;
   virtual void disableBlending() = 0;
   virtual void useTransparentBlackBlending() = 0;
   virtual void useSpyBugBlending() = 0;
   virtual void useDefaultBlending() = 0;
   virtual void enableDepthTest() = 0;
   virtual void disableDepthTest() = 0;

   virtual void enableStencil() = 0;
   virtual void disableStencil() = 0;
   virtual void useAndStencilTest() = 0;
   virtual void useNotStencilTest() = 0;
   virtual void enableStencilDrawOnly() = 0;
   virtual void disableStencilDraw() = 0;

   virtual void setViewport(S32 x, S32 y, S32 width, S32 height) = 0;
   virtual Point getViewportPos() = 0;
   virtual Point getViewportSize() = 0;

   virtual void enableScissor() = 0;
   virtual void disableScissor() = 0;
   virtual bool isScissorEnabled() = 0;
   virtual void setScissor(S32 x, S32 y, S32 width, S32 height) = 0;
   virtual Point getScissorPos() = 0;
   virtual Point getScissorSize() = 0;

   // Matrix transforms
   virtual void scale(F32 x, F32 y, F32 z = 1.0f) = 0;
   virtual void translate(F32 x, F32 y, F32 z = 0.0f) = 0;
   virtual void rotate(F32 degAngle, F32 x, F32 y, F32 z) = 0;

   virtual void setMatrixMode(MatrixType type) = 0;
   virtual void getMatrix(MatrixType type, F32* matrix) = 0;
   virtual void pushMatrix() = 0;
   virtual void popMatrix() = 0;
   virtual void loadMatrix(const F32* m) = 0;
   virtual void loadMatrix(const F64* m) = 0;
   virtual void loadIdentity() = 0;
   virtual void projectOrtho(F32 left, F32 right, F32 bottom, F32 top, F32 nearZ, F32 farZ) = 0;

   // Textures
   virtual U32 generateTexture(bool useLinearFiltering = true) = 0;
   virtual void bindTexture(U32 textureHandle) = 0;
   virtual bool isTexture(U32 textureHandle) = 0;
   virtual void deleteTexture(U32 textureHandle) = 0;
   virtual void setTextureData(TextureFormat format, DataType dataType, U32 width, U32 height, const void* data) = 0;
   virtual void setSubTextureData(TextureFormat format, DataType dataType, S32 xOffset, S32 yOffset,
                                  U32 width, U32 height, const void* data) = 0;

   // Framebuffers
   virtual void readFramebufferPixels(TextureFormat format, DataType dataType, S32 x, S32 y, S32 width, S32 height, void* data) = 0;

   // Render points:
   virtual void renderVertexArray(const S8 verts[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2) = 0;
   virtual void renderVertexArray(const S16 verts[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2) = 0;
   virtual void renderVertexArray(const F32 verts[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2) = 0;

   // Render RGBA colored vertices
   virtual void renderColored(const F32 verts[], const F32 colors[], U32 vertCount,
      RenderType type, U32 start = 0, U32 stride = 0, U32 vertDimension = 2) = 0;     // Same stride is used for both verts and colors

   virtual void renderTextured(const F32 verts[], const F32 UVs[], U32 vertCount,
      RenderType type, U32 start = 0, U32 stride = 0, U32 vertDimension = 2) = 0;     // Same stride is used for both verts and UVs

   // Render with a texture colored by the current color.
   // Use isAlphaTexture for coloring based purely on the texture's alpha values (texture color is ignored).
   virtual void renderColoredTexture(const F32 verts[], const F32 UVs[], U32 vertCount, RenderType type,
      U32 start = 0, U32 stride = 0, U32 vertDimension = 2, bool isAlphaTexture = false) = 0;
};


}

#endif /* _RENDERER_H_ */