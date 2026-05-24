//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "../tnl/tnlTypes.h"
#include "glad/glad.h"
#include <vector>
using namespace TNL;

// Forward declaration — the full nanosvg.h (with NANOSVG_IMPLEMENTATION) is
// included only in pictureloader.cpp to avoid the header guard defeating the
// implementation block.
struct NSVGimage;

struct PictureLoader
{
   U32 x;
   U32 y;
   U32 *data;
   PictureLoader() {x=0; y=0; data=NULL;}
   virtual ~PictureLoader() {if(data) delete data;}
};

PictureLoader *LoadPicture(const char* path);
PictureLoader *LoadPictureSVG(const char* path, int targetHeight);
GLuint loadGLTexSVG(const char* path, int baseSize = 256);
GLuint loadGLTex(PictureLoader *picture);

// A tessellated SVG ready for geometry rendering
struct SvgContour
{
   std::vector<float> pts;   // interleaved x,y
   bool closed;
};

struct SvgGeometry
{
   std::vector<SvgContour> contours;
   float width;    // original SVG viewport width
   float height;   // original SVG viewport height
};

SvgGeometry *LoadSvgGeometry(const char* path, int steps = 16);


extern bool LoadWAVFile(const char *filename, char &format, char **data, int &size, int &freq);
