#ifndef _AL_BUFFER_H_
#define _AL_BUFFER_H_

#include "AL/al.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_PADDING 2

typedef struct ALbuffer
{
    ALfloat *data;
    ALsizei  size;

    ALenum   format;
    ALenum   eOriginalFormat;
    ALsizei  frequency;

    ALuint   refcount; // Number of sources using this buffer (deletion can only occur when this is 0)

    // Index to itself
    ALuint buffer;

    struct ALbuffer *next;
} ALbuffer;

ALvoid ReleaseALBuffers(ALCdevice *device);

AL_API ALvoid AL_APIENTRY alGenBuffers(ALsizei n,ALuint *puiBuffers);
AL_API ALvoid AL_APIENTRY alDeleteBuffers(ALsizei n, const ALuint *puiBuffers);
AL_API ALboolean AL_APIENTRY alIsBuffer(ALuint uiBuffer);
AL_API ALvoid AL_APIENTRY alBufferData(ALuint buffer,ALenum format,const ALvoid *data,ALsizei size,ALsizei freq);
AL_API ALvoid AL_APIENTRY alBufferSubDataEXT(ALuint buffer,ALenum format,const ALvoid *data,ALsizei offset,ALsizei length);
AL_API void AL_APIENTRY alBufferf(ALuint buffer, ALenum eParam, ALfloat flValue);
AL_API void AL_APIENTRY alBuffer3f(ALuint buffer, ALenum eParam, ALfloat flValue1, ALfloat flValue2, ALfloat flValue3);
AL_API void AL_APIENTRY alBufferfv(ALuint buffer, ALenum eParam, const ALfloat* flValues);
AL_API void AL_APIENTRY alBufferi(ALuint buffer, ALenum eParam, ALint lValue);
AL_API void AL_APIENTRY alBuffer3i( ALuint buffer, ALenum eParam, ALint lValue1, ALint lValue2, ALint lValue3);
AL_API void AL_APIENTRY alBufferiv(ALuint buffer, ALenum eParam, const ALint* plValues);
AL_API ALvoid AL_APIENTRY alGetBufferf(ALuint buffer, ALenum eParam, ALfloat *pflValue);
AL_API void AL_APIENTRY alGetBuffer3f(ALuint buffer, ALenum eParam, ALfloat* pflValue1, ALfloat* pflValue2, ALfloat* pflValue3);
AL_API void AL_APIENTRY alGetBufferfv(ALuint buffer, ALenum eParam, ALfloat* pflValues);
AL_API ALvoid AL_APIENTRY alGetBufferi(ALuint buffer, ALenum eParam, ALint *plValue);
AL_API void AL_APIENTRY alGetBuffer3i(ALuint buffer, ALenum eParam, ALint* plValue1, ALint* plValue2, ALint* plValue3);
AL_API void AL_APIENTRY alGetBufferiv(ALuint buffer, ALenum eParam, ALint* plValues);

#ifdef __cplusplus
}
#endif

#endif
