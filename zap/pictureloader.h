#include "../tnl/tnlTypes.h"
using namespace TNL;

struct pictureLoader
{
   U32 x;
   U32 y;
   U32 *data;
   pictureLoader() {x=0; y=0; data=NULL;}
   ~pictureLoader() {if(data) delete data;}
};

extern bool setGLTex(S32 num);