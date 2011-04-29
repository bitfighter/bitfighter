#ifndef _AL_BUFFER_H_
#define _AL_BUFFER_H_

#include "AL/al.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Input formats (some are currently theoretical) */
enum UserFmtType {
    UserFmtByte,   /* AL_BYTE */
    UserFmtUByte,  /* AL_UNSIGNED_BYTE */
    UserFmtShort,  /* AL_SHORT */
    UserFmtUShort, /* AL_UNSIGNED_SHORT */
    UserFmtInt,    /* AL_INT */
    UserFmtUInt,   /* AL_UNSIGNED_INT */
    UserFmtFloat,  /* AL_FLOAT */
    UserFmtDouble, /* AL_DOUBLE */
    UserFmtMulaw,  /* AL_MULAW */
    UserFmtIMA4,   /* AL_IMA4 */
};
enum UserFmtChannels {
    UserFmtMono,   /* AL_MONO */
    UserFmtStereo, /* AL_STEREO */
    UserFmtRear,   /* AL_REAR */
    UserFmtQuad,   /* AL_QUAD */
    UserFmtX51,    /* AL_5POINT1 (WFX order) */
    UserFmtX61,    /* AL_6POINT1 (WFX order) */
    UserFmtX71,    /* AL_7POINT1 (WFX order) */
};

ALboolean DecomposeUserFormat(ALenum format, enum UserFmtChannels *chans,
                              enum UserFmtType *type);
ALuint BytesFromUserFmt(enum UserFmtType type);
ALuint ChannelsFromUserFmt(enum UserFmtChannels chans);
static __inline ALuint FrameSizeFromUserFmt(enum UserFmtChannels chans,
                                            enum UserFmtType type)
{
    return ChannelsFromUserFmt(chans) * BytesFromUserFmt(type);
}


/* Storable formats */
enum FmtType {
    FmtUByte = UserFmtUByte,
    FmtShort = UserFmtShort,
    FmtFloat = UserFmtFloat,
};
enum FmtChannels {
    FmtMono = UserFmtMono,
    FmtStereo = UserFmtStereo,
    FmtRear = UserFmtRear,
    FmtQuad = UserFmtQuad,
    FmtX51 = UserFmtX51,
    FmtX61 = UserFmtX61,
    FmtX71 = UserFmtX71,
};

ALboolean DecomposeFormat(ALenum format, enum FmtChannels *chans, enum FmtType *type);
ALuint BytesFromFmt(enum FmtType type);
ALuint ChannelsFromFmt(enum FmtChannels chans);
static __inline ALuint FrameSizeFromFmt(enum FmtChannels chans, enum FmtType type)
{
    return ChannelsFromFmt(chans) * BytesFromFmt(type);
}


typedef struct ALbuffer
{
    ALvoid  *data;
    ALsizei  size;

    ALsizei          Frequency;
    enum FmtChannels FmtChannels;
    enum FmtType     FmtType;

    enum UserFmtChannels OriginalChannels;
    enum UserFmtType     OriginalType;
    ALsizei OriginalSize;
    ALsizei OriginalAlign;

    ALsizei  LoopStart;
    ALsizei  LoopEnd;

    ALuint   refcount; // Number of sources using this buffer (deletion can only occur when this is 0)

    // Index to itself
    ALuint buffer;
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
