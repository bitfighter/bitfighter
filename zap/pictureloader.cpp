#include "pictureloader.h"
#include "tnlLog.h"

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



PictureLoader *LoadPicture(const char* path){
   int a,b,c,d,e,j,x,y,vx,bpp,x2,y2;
   U32 p[256];
   U32 *mem;
   PictureLoader *pict = NULL;
   void* r=readfile((const char*) path);
   if(r==0) return 0;
   readshort(r);
   readint(r); //don't need some info.
   readint(r);
   readint(r);
   readint(r);
   x=((readint(r)-1) & 0xFFFF)+1;
   y=((readint(r)-1) & 0xFFFF)+1;
   pict = new PictureLoader();
   pict->data = new U32[x*y];
   pict->x = x;
   pict->y = y;
   mem = pict->data;
   readshort(r);
   bpp=((readshort(r)-1) & 31)+1;
   readint(r);
   readint(r);
   readint(r);
   readint(r);
   readint(r);
   readint(r);
   switch(bpp){ //bmp is lined up by 32's
    case 32:vx=0; //32 bpp never have blank bytes
    break;case 16:vx=-(x << 1) & 3;
    break;case 8:vx= -x & 3;
    break;case 4:vx=-((x+1) >> 1) & 3;
    break;case 2:vx=-((x+3) >> 2) & 3;
    break;case 1:vx=-((x+7) >> 3) & 3;
    break;default:vx=-(x*3) & 3;bpp=24; //pretend unsupported formats is 24 bpp
    break;
   }
   if(bpp<=8){
      a=1 << bpp;
      if(!p) {closefile(r);return 0;}
      for(b=0;b<a;b++){
         p[b]=readint(r) ^ 0xFF000000; //most palette are 32 bit with zero alpha.
      }
   }

   c=0;
   d=0;
   y2=y;while(y2>0){
      y2--;e=0;
      x2=0;while(x2<x){
         switch(bpp){
         case 32:j=readint(r);
         break;case 24:j=readshort(r);j |= readbyte(r) << 16 | 0xFF000000;
         break;case 16:j=readshort(r);j=j | (j & 255) << 16 | 0xFF000000;
         break;case 8:j=p[readbyte(r)];
         break;default:
            if(!c){e=8-bpp;c=255;d=readbyte(r);}
            j=p[(d & c) >> e];
            e-=bpp;c=c >> bpp;
         break;
         }
         //If j And $FF000000 Xor $FF000000 Then K5=K5 Or $40000000
         //B4=j And $F0F0F0F0:If (B4 Shr 4 Or B4) Xor j Then K5=K5 And $DFFFFFFF
         //If j Xor M5 Else j=j And $FFFFFF

         j = (j & 255) << 16 | ((j >> 16) & 255) | (j & 0xFF00FF00);  // BGRA to RGBA, as used for GL Texture loader
         mem[x2+y2*x]=j;
         x2++;
      }
      for(e=1;e<vx;e++){a=readbyte(r);}
      //If Eof(r) Then y2=0
   }
   closefile(r);
   return pict;
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

