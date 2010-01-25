/*
This file is part of Yars' Exile.

Yars' Exile is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Yars' Exile is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Yars' Exile.  If not, see <http://www.gnu.org/licenses/>.

Copyright 2009 Average Software
*/

// Average Software Sound Wrapper

#include "SoundWrapper.h"
#include "Base.h"
#include <fstream>
#include <vector>
#include <map>
#include <vorbis/vorbisfile.h>
#include <vorbis/codec.h>
#include <cstdio>

#ifdef __APPLE__

#include <OpenAL/al.h>
#include <OpenAL/alc.h>

#elif defined __WIN32__

#include <AL/al.h>
#include <AL/alc.h>
#include <windows.h>

#elif defined __UNIX__

#include <AL/al.h>
#include <AL/alc.h>

#else

#error No system defined in SoundWrapper.cpp!

#endif

using namespace ASBase;
using namespace SoundWrapper;

using std::string;
using std::vector;
using std::map;
using std::ifstream;
using std::FILE;
using std::fopen;
using std::fclose;
using std::bad_alloc;

namespace
{
    // Object representing a sound effect.
    struct Sound
    {
        ~Sound() throw ();
        
        // The OpenAL buffer associated with this sound.
        ALuint buffer;
        // The OpenAL source associated with this sound.
        ALuint source;
    };
    
    // Object representing an audio stream.
    struct AudioStream
    {
        ~AudioStream() throw ();
        
        // The stream is triple buffered.
        ALuint buffers[3];
        // The OpenAL source for the stream.
        ALuint source;
        // The file handle used by the Ogg Vorbis API.
        OggVorbis_File file_handle;
        // OpenAL format identifier, used for buffering.
        ALenum format;
        // OpenAL frequency, used for buffering.
        ALsizei frequency;
        // The point, in seconds, that the stream returns to after
        // reaching the end of the file.
        double loop_point;
    };
    
    // This is responsible for filling the stream buffer
    // identified by the parameter.
    void FillBuffer(ALuint al_buffer) throw ();
    // This scans the stored sound IDs and locates the next
    // available one.
    SoundID NextSoundID() throw ();
    
    // Handle to the OpenAL device.
    ALCdevice *sound_device;
    // Handle to the OpenAL context.
    ALCcontext *sound_context;
    // Ogg Vorbis needs to know the host system's endianness.
    const int endianness = bitwise_cast<char>(1) ^ 1;
    // Tracks the on/off state of the sound system.
    bool enabled;
    // The internal list of sounds.
    map<SoundID, Sound*> sound_list;
    // The current music stream.
    AudioStream *stream;
}

#ifndef EXPORT_ENABLED
namespace ASBase
{
    namespace SoundWrapper
    {
        // Redeclaring this function for internal use, since the
        // declaration in the header only applies to compilers
        // that support export.
        template <typename type>
        void DeleteSounds(type begin, type end);
    }
}
#endif

void SoundWrapper::Initialize() throw (InitializeFailure)
{
    // First attempt to open the sound device.
	sound_device = alcOpenDevice(NULL);
	
	if (sound_device == NULL)
	{
		throw InitializeFailure("Failed to open sound device!", NULL, "SoundWrapper::Initialize");
	}
	
    // Attempt to create a sound context.
	sound_context = alcCreateContext(sound_device, NULL);
	
	if (sound_context == NULL || !alcMakeContextCurrent(sound_context))
	{
		throw InitializeFailure("Failed to obtain context!", NULL, "SoundWrapper::Initialize");
	}
	
    // Reset the error state.
	alGetError();
	
    // Set up the listener in the middle of the screen,
    // facing down the z axis.
	ALfloat orientation[6] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};
	
	alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
	alListener3f(AL_VELOCITY, 0.0, 0.0, 0.0);
	alListenerfv(AL_ORIENTATION, orientation);
	
    enabled = true;
}

void SoundWrapper::Shutdown() throw (ShutdownFailure)
{
    // Clean up all sound data.
    ClearSounds();
    
    // Release the context.
	alcMakeContextCurrent(NULL);
	alcDestroyContext(sound_context);
	
    // Attempt to close the device.
	if (!alcCloseDevice(sound_device))
	{
		throw ShutdownFailure("Failed to close sound device!", NULL, "SoundWrapper::Shutdown");
	}
}

SoundID SoundWrapper::LoadOggVorbis(string filename) throw (FileNotFound, bad_alloc)
{	
    // Open the Ogg Vorbis file.
	FILE *in_file = fopen(filename.c_str(), "rb");
	
    // Make sure we got it.
	if (in_file == NULL)
	{
		throw FileNotFound(filename, NULL, "SoundWrapper::LoadOggVorbis");
	}
	
    // Create a new Sound object.
	Sound *new_sound = new Sound;
	vorbis_info *v_info;
	OggVorbis_File ov_file;

    // Set up the Ogg Vorbis decoder with the file.
	ov_open_callbacks(in_file, &ov_file, NULL, 0, OV_CALLBACKS_DEFAULT);
    // Get some information about the sound format.
	v_info = ov_info(&ov_file, -1);
	
	ALenum format;
    ALsizei frequency;
	
    // Convert the Ogg Vorbis information into the format that OpenAL wants.
	format = v_info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	frequency = v_info->rate;
	
    // This vector will hold the decoded sound data.
	vector<char> data_buffer;
	long bytes;
	
	do
	{
		char array[32768];
        // This serves no actual purpose, ov_read returns something in it.
		int bit_stream;
		
        // Read a chunk of the file into array.
		bytes = ov_read(&ov_file, array, 32768, endianness, 2, 1, &bit_stream);
        // Insert the data into the final buffer.
		data_buffer.insert(data_buffer.end(), array, array + bytes);
        // Keep going until all the data is read.
	} while (bytes > 0);
	
    // Close the file.
	ov_clear(&ov_file);
	
    // Create new OpenAL constructs for the sound effect.
	alGenBuffers(1, &new_sound->buffer);
	alGenSources(1, &new_sound->source);

    // Buffer the data.
	alBufferData(new_sound->buffer, format, &data_buffer[0], data_buffer.size(), frequency);
    // Set up the sound properties.
    alSourcef(new_sound->source, AL_PITCH, 1.0);
    alSourcef(new_sound->source, AL_GAIN, 1.0);
    alSource3f(new_sound->source, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(new_sound->source, AL_VELOCITY, 0.0, 0.0, 0.0);
    alSourcei(new_sound->source, AL_LOOPING, AL_FALSE);
	alSourcei(new_sound->source, AL_BUFFER, new_sound->buffer);
	
    // Get the new sound ID.
    SoundID new_id = NextSoundID();
    
    // Add the sound to the list.
	sound_list[new_id] = new_sound;
	
    // Return the ID number of the new sound.
	return new_id;
}

void SoundWrapper::LoadOggVorbisStream(string filename) throw (FileNotFound, bad_alloc)
{
    // Open the Ogg Vorbis file.
	FILE *ogg_file = fopen(filename.c_str(), "rb");
	
    // Make sure we got it.
	if (ogg_file == NULL)
	{
		throw FileNotFound(filename, NULL, "SoundWrapper::LoadOggVorbisStream");
	}
	
    // Build the name of the loop file and open it.
	string loop_file = filename.substr(0, filename.length() - 3) + "loop";
	ifstream in_file(loop_file.c_str());
	
	// Delete the old audio stream and make a new one.
	delete stream;
	stream = new AudioStream;
	
    // Check to see if the .loop file was opened.
    if (in_file)
    {
        // If so, load the loop point.
        in_file >> stream->loop_point;
        
        // If the read failed...
        if (!in_file)
        {
            // Assume 0.0.
            stream->loop_point = 0.0;
        }
    }
    else
    {
        // If the .loop file doesn't exist, assume 0.0.
        stream->loop_point = 0.0;
    }

	vorbis_info *v_info;
	
    // Hand the file handle to Ogg Vorbis.
	ov_open_callbacks(ogg_file, &stream->file_handle, NULL, 0, OV_CALLBACKS_DEFAULT);
    // Get some info about the file.
	v_info = ov_info(&stream->file_handle, -1);
    // Convert the info to OpenAL format.
	stream->format = v_info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	stream->frequency = v_info->rate;
	
    // Create three buffers and a source for the stream.
	alGenBuffers(3, stream->buffers);
	alGenSources(1, &stream->source);
	
    // Fill all three buffers.
	FillBuffer(stream->buffers[0]);
	FillBuffer(stream->buffers[1]);
	FillBuffer(stream->buffers[2]);
	
    // Set the various stream properties.
	alSourcef(stream->source, AL_PITCH, 1.0);
    alSourcef(stream->source, AL_GAIN, 1.0);
    alSource3f(stream->source, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(stream->source, AL_VELOCITY, 0.0, 0.0, 0.0);
	alSource3f(stream->source, AL_DIRECTION, 0.0, 0.0, 0.0);
    alSourcei(stream->source, AL_LOOPING, AL_FALSE);
	alSourcef(stream->source, AL_ROLLOFF_FACTOR, 0.0);
    // Queue the three buffers.
	alSourceQueueBuffers(stream->source, 3, stream->buffers);
}

void SoundWrapper::PlaySound(SoundID sound) throw ()
{
	if (!enabled)
	{
		return;
	}
	
	alSourcePlay(sound_list[sound]->source);
}

void SoundWrapper::PlayStream() throw ()
{
	if (!enabled || stream == NULL)
	{
		return;
	}
	
	alSourcePlay(stream->source);
}

void SoundWrapper::PauseStream() throw ()
{
	if (!enabled || stream == NULL)
	{
		return;
	}
	
	alSourcePause(stream->source);
}

void SoundWrapper::RewindStream() throw ()
{
	if (!enabled || stream == NULL)
	{
		return;
	}
	
    // Stop the stream.
	alSourceStop(stream->source);
    // Go back to position 0.
	ov_raw_seek(&stream->file_handle, 0);
    // Unqueue the buffers.
	alSourceUnqueueBuffers(stream->source, 3, stream->buffers);
	
    // Fill the buffers again from the beginning of the stream.
	FillBuffer(stream->buffers[0]);
	FillBuffer(stream->buffers[1]);
	FillBuffer(stream->buffers[2]);
	
    // Queue the buffers up again.
	alSourceQueueBuffers(stream->source, 3, stream->buffers);
}

void SoundWrapper::ProcessStream() throw ()
{
	if (!enabled || stream == NULL)
	{
		return;
	}
	
	int buffers_processed;
	int state;
	
    // Get the stream state.
	alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
	
    // If the stream was stopped, we want to fill all three buffers again.
	if (state == AL_STOPPED)
	{
		buffers_processed = 3;
	}
	else
	{
        // Otherwise, request the number of empty buffers.
		alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &buffers_processed);
        
        // If none of the buffers have finished playing, get out.
        if (buffers_processed == 0)
        {
            return;
        }
	}
	
	vector<ALuint> buffers(buffers_processed);

    // Unqueue the completed buffers into the vector.
	alSourceUnqueueBuffers(stream->source, buffers_processed, &buffers[0]);
	
    // Fill them with fresh data.
	for (int x = 0; x < buffers_processed; x++)
	{
		FillBuffer(buffers[x]);
	}
	
    // Queue them back up.
	alSourceQueueBuffers(stream->source, buffers_processed, &buffers[0]);
	
    // If the stream was stopped, start it up again.
	if (state == AL_STOPPED)
	{
		alSourcePlay(stream->source);
	}
}

void SoundWrapper::Enable() throw ()
{
    enabled = true;
}

void SoundWrapper::Disable() throw ()
{
    enabled = false;
}

void SoundWrapper::ToggleEnabled() throw ()
{
    enabled = !enabled;
}

void SoundWrapper::ClearSounds() throw ()
{
	for (map<SoundID, Sound*>::const_iterator i = sound_list.begin(); i != sound_list.end(); ++i)
    {
        delete i->second;
    }
	
	sound_list.clear();
	delete stream;
	stream = NULL;
}

void SoundWrapper::DeleteSound(SoundID sound) throw ()
{
    // Free the sound object.
    delete sound_list[sound];
    // Remove the entry.
    sound_list.erase(sound);
}

template <typename type>
void SoundWrapper::DeleteSounds(type begin, type end)
{
    while (begin != end)
    {
        DeleteSound(*begin);
        ++begin;
    }
}

namespace
{
    Sound::~Sound() throw ()
    {
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        alDeleteBuffers(1, &buffer);
        alDeleteSources(1, &source);
    }

    AudioStream::~AudioStream() throw ()
    {
        alSourceStop(source);
        alSourcei(source, AL_BUFFER, 0);
        alDeleteBuffers(3, buffers);
        alDeleteSources(1, &source);
        ov_clear(&file_handle);
    }
    
    void FillBuffer(ALuint al_buffer) throw ()
    {
        char buffer[32768];
        ALuint total_bytes = 0;
        
        do
        {
            int bytes = 0;
            int bit_stream;
            
            // Read a chunk of data.
            bytes = ov_read(&stream->file_handle, buffer + total_bytes, 32768 - total_bytes, endianness, 2, 1, &bit_stream);
            
            // If the file is at the end...
            if (bytes == 0)
            {
                // Go back to the loop point.
                ov_time_seek(&stream->file_handle, stream->loop_point);
                
                continue;
            }
            
            total_bytes += bytes;
            //Loop until the buffer is full.
        } while (total_bytes < 32768);
        
        // Put the data in the OpenAL buffer.
        alBufferData(al_buffer, stream->format, buffer, total_bytes, stream->frequency);
    }
    
    SoundID NextSoundID() throw ()
    {
        // Assume it's 0.
        SoundID next = 0;
        
        for (map<SoundID, Sound*>::const_iterator i = sound_list.begin(); i != sound_list.end(); ++i)
        {
            // If this number is not taken, return it.
            if (i->first != next)
            {
                return next;
            }
            
            // Look for the next number.
            next++;
        }
        
        return next;
    }
}
