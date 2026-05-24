//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#include "pictureloader.h"
#include "tnlLog.h"

#include "glad/glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../fontstash/stb_image.h"

#define NANOSVG_IMPLEMENTATION
#include "../fontstash/nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "../fontstash/nanosvgrast.h"

#include <stdio.h>                  // For file reading


#ifdef _MSC_VER
#pragma warning (disable: 4996)     // Kill warnings about fopen!
#endif


void* readfile(const char* filename){
    void* fp;
   fp=fopen((const char*)filename,"rb");
   //getc(fp);
    return fp;
}


void closefile(void* fp){
   fclose((FILE*)fp);
}

unsigned char readbyte(void* fp){
   int a=getc((FILE*)fp);
   if(a==EOF) return 0;
   return (unsigned char)a;
}
unsigned short readshort(void* fp){
   int a=getc((FILE*)fp);
   if(a==EOF) return 0;
   return (unsigned short)(a | (getc((FILE*)fp) << 8));
}
unsigned int readint(void* fp){
   int a=getc((FILE*)fp);
   if(a==EOF) return 0;
//   return a | (getc((FILE*)fp) << 8) //stupid optimizer calling function at wrong order.
//       | (getc((FILE*)fp) << 16)    //no wonder why it only works right in debug mode.
//        | (getc((FILE*)fp) << 24);
   a|=getc((FILE*)fp) << 8;
   a|=getc((FILE*)fp) << 16;
   return a | getc((FILE*)fp) << 24;
}



bool LoadWAVFile(const char *filename, char &format, char **data, int &size, int &freq)
{
   void *file = readfile(filename);
   if(file == NULL)
      return false;

   int a;

   if(readint(file) != 0x46464952)
   {
      closefile(file);
      return false;
   }
   readint(file); // size
   readint(file); // "WAVE"
   readint(file); // "fmt "
   int size1 = readint(file) & 255; // size of format code  (linit to avoid freezing)
   readshort(file); // format
   bool stereo = readshort(file) == 2;
   freq = readint(file);
   readint(file); // data rate
   readshort(file); // data block size
   bool bits16 = readshort(file) == 16; // bits per sample
   for(int i=16; i<size1; i++)
      readbyte(file);

   a=readint(file);
   while(a != 0x61746164 && a != 0) // loop until found "data"
   {
      a=readint(file);
      for(int i=0; i < (a & 0xFFF); i++)
         readbyte(file);
      a=readint(file);
   }
   size = readint(file);
   if(size > 0x8000000) size = 0x8000000; // limit 128 MB
   if(size < 1)
   {
      closefile(file);
      return false;
   }

   *data = new char[size];
   size_t readsize = fread(*data, 1, size, (FILE*) file);
   closefile(file);
   format = (stereo ? 2 : 0) + (bits16 ? 1 : 0);
   if(readsize < 1)
   {
      delete *data;
      return false;
   }
   return true;
}



PictureLoader *LoadPicture(const char* path)
{
   int x, y, channels;
   unsigned char *pixels = stbi_load(path, &x, &y, &channels, 4);  // force RGBA
   if(!pixels)
      return NULL;

   PictureLoader *pict = new PictureLoader();
   pict->x = (U32)x;
   pict->y = (U32)y;
   pict->data = new U32[x * y];
   memcpy(pict->data, pixels, x * y * 4);
   stbi_image_free(pixels);
   return pict;
}


PictureLoader *LoadPictureSVG(const char* path, int targetHeight)
{
   NSVGimage *svg = nsvgParseFromFile(path, "px", 96.0f);
   if(!svg)
      return NULL;

   float scale = (float)targetHeight / svg->height;
   int w = (int)(svg->width  * scale);
   int h = targetHeight;

   unsigned char *pixels = new unsigned char[w * h * 4];
   NSVGrasterizer *rast = nsvgCreateRasterizer();
   nsvgRasterize(rast, svg, 0, 0, scale, pixels, w, h, w * 4);
   nsvgDeleteRasterizer(rast);
   nsvgDelete(svg);

   PictureLoader *pict = new PictureLoader();
   pict->x = (U32)w;
   pict->y = (U32)h;
   pict->data = new U32[w * h];
   memcpy(pict->data, pixels, w * h * 4);
   delete[] pixels;
   return pict;
}


// Rasterizes an SVG at each mip level directly from NanoSVG and uploads all levels
// to a new GL texture with trilinear filtering.  Returns the GL texture handle, or 0 on failure.
GLuint loadGLTexSVG(const char* path, int baseSize)
{
   NSVGimage *svg = nsvgParseFromFile(path, "px", 96.0f);
   if(!svg)
      return 0;

   float aspect = svg->width / svg->height;

   GLuint tex;
   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   NSVGrasterizer *rast = nsvgCreateRasterizer();
   int level = 0;
   for(int h = baseSize; h >= 1; h >>= 1, level++)
   {
      int w = (int)(h * aspect);
      if(w < 1) w = 1;

      float scale = (float)h / svg->height;
      unsigned char *pixels = new unsigned char[w * h * 4];
      nsvgRasterize(rast, svg, 0, 0, scale, pixels, w, h, w * 4);

      // Set RGB to white, preserve SVG alpha for color tinting
      for(int i = 0; i < w * h; i++)
      {
         U8 alpha = pixels[i * 4 + 3];
         pixels[i * 4 + 0] = 255;
         pixels[i * 4 + 1] = 255;
         pixels[i * 4 + 2] = 255;
         pixels[i * 4 + 3] = alpha;
      }

      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      delete[] pixels;
   }

   nsvgDeleteRasterizer(rast);
   nsvgDelete(svg);

   return tex;
}


// Evaluate a cubic bezier at parameter t, returning x or y component.
// p0,p1,p2,p3 are the control point coordinates for that axis.
static float cubicBezier(float p0, float p1, float p2, float p3, float t)
{
   float u = 1.0f - t;
   return u*u*u*p0 + 3*u*u*t*p1 + 3*u*t*t*p2 + t*t*t*p3;
}

SvgGeometry *LoadSvgGeometry(const char* path, int steps)
{
   NSVGimage *svg = nsvgParseFromFile(path, "px", 96.0f);
   if(!svg)
      return NULL;

   SvgGeometry *geom = new SvgGeometry();
   geom->width  = svg->width;
   geom->height = svg->height;

   for(NSVGshape *shape = svg->shapes; shape != NULL; shape = shape->next)
   {
      for(NSVGpath *path = shape->paths; path != NULL; path = path->next)
      {
         SvgContour contour;
         contour.closed = path->closed != 0;

         // pts layout: x0,y0, then groups of cpx1,cpy1,cpx2,cpy2,x1,y1
         // npts counts individual points (pairs), so segment count = (npts-1)/3
         int nsegs = (path->npts - 1) / 3;
         for(int s = 0; s < nsegs; s++)
         {
            float *p = path->pts + s * 6;  // each segment advances by 6 floats
            float x0=p[0], y0=p[1], cx1=p[2], cy1=p[3], cx2=p[4], cy2=p[5], x1=p[6], y1=p[7];

            int emit = (s == 0) ? steps + 1 : steps;  // include start point for first seg only
            int start = (s == 0) ? 0 : 1;
            for(int i = start; i <= steps; i++)
            {
               float t = (float)i / (float)steps;
               contour.pts.push_back(cubicBezier(x0, cx1, cx2, x1, t));
               contour.pts.push_back(cubicBezier(y0, cy1, cy2, y1, t));
            }
         }

         if(!contour.pts.empty())
            geom->contours.push_back(contour);
      }
   }

   nsvgDelete(svg);
   return geom;
}


GLuint loadGLTex(PictureLoader *picture)
{
   GLuint outputGL;

   /* Get a font index from OpenGL */
   glGenTextures(1, &outputGL);    /* Create 1 texture, store in glFontHandle */
   {int err=glGetError();if(err)printf("glGenTextures() error: %i\n",err);}
    
   /* Select our font */
   glBindTexture(GL_TEXTURE_2D, outputGL);
   {int err=glGetError();if(err)printf("glBindTexture() error: %i\n",err);}

   /* Set some parameters i guess */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


   //if(num == 0)  // move color into alpha
   //{
   //   for(S32 i = pict->x * pict->y - 1; i>=0; i--)
   //      pict->data[i] = (pict->data[i] << 24) | 0x00FFFFFF;
   //}

   glTexImage2D(
         GL_TEXTURE_2D, 0, GL_RGBA,
         picture->x, picture->y, 0,
         GL_RGBA, GL_UNSIGNED_BYTE, picture->data);
   {int err=glGetError();if(err)printf("glBindTexture() error: %i\n",err);}
   return outputGL;
}

