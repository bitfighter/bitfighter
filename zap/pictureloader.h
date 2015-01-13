//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "tnlTypes.h"

#if defined(TNL_OS_MOBILE) || defined(BF_USE_GLES)
#include "SDL_opengles.h"
#else
#include "SDL_opengl.h"
#endif

using namespace TNL;

struct PictureLoader
{
   U32 x;
   U32 y;
   U32 *data;
   PictureLoader() {x=0; y=0; data=NULL;}
   virtual ~PictureLoader() {if(data) delete data;}
};

PictureLoader *LoadPicture(const char* path);
GLuint loadGLTex(PictureLoader picture);


extern bool LoadWAVFile(const char *filename, char &format, char **data, int &size, int &freq);
