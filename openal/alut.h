#ifndef _ALUT_H_
#define _ALUT_H_

#define ALUTAPI
#define ALUTAPIENTRY __cdecl

#if defined(__linux)
#undef ALUTAPIENTRY
#define ALUTAPIENTRY
typedef ALCvoid ALCdevice;
typedef ALCvoid ALCcontext;
#endif

#include "al.h"
#include "alu.h"

#ifdef __cplusplus
extern "C" {
#endif

ALUTAPI ALvoid	ALUTAPIENTRY alutInit(ALint *argc,ALbyte **argv);
ALUTAPI ALvoid	ALUTAPIENTRY alutExit();
ALUTAPI ALvoid	ALUTAPIENTRY alutLoadWAVFile(ALbyte *file,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop);
ALUTAPI ALvoid	ALUTAPIENTRY alutLoadWAVMemory(ALbyte *memory,ALenum *format,ALvoid **data,ALsizei *size,ALsizei *freq,ALboolean *loop);
ALUTAPI ALvoid  ALUTAPIENTRY alutUnloadWAV(ALenum format,ALvoid *data,ALsizei size,ALsizei freq);

#ifdef __cplusplus
}
#endif

#endif
