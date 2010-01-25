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

// This package provides a simple interface to OpenAL.  It manages the loading
// and playing of both sound effects and streaming music.  At present, only
// Ogg Vorbis format files are supported.

#ifndef SOUNDWRAPPER_H
#define SOUNDWRAPPER_H

#include "FileNotFound.h"
#include "InitializeFailure.h"
#include "ShutdownFailure.h"
#include <string>
#include <new>
#include <cstddef>

namespace ASBase
{
	namespace SoundWrapper
	{
        // Type used to identify sound effects.
        typedef std::size_t SoundID;
        
        // Initializes OpenAL and the internal data structures.
        // This MUST be called before using anything else in the package.
        void Initialize() throw (InitializeFailure);
        
        // Shuts down OpenAL and releases all resources used by loaded sounds.
        // Should be called just before the program exits.
		void Shutdown() throw (ShutdownFailure);
        
        // Attempts to load the Ogg Vorbis file specified by the parameter.
        // Returns the ID of the new sound effect if successful.
        // Throws a FileNotFound if the file doesn't exist, throws a
        // bad_alloc if memory is exhausted.
		SoundID LoadOggVorbis(std::string filename) throw (FileNotFound, std::bad_alloc);
        
        // Prepares an Ogg Vorbis file for streaming playback.  The package
        // maintains one stream connection.  The file specified by the parameter
        // is expected to have a corresponding .loop file in the same directory.
        // This file should contain a single number that indicates the point
        // to loop back to after the stream reaches the end of the file.
        // This value is expressed in seconds.  If the .loop file doesn't exist,
        // a loop point of 0.0 (the beginning of the file) is assumed. Throws
        // FileNotFound if the specified file or is not found, and bad_alloc
        // on memory exhaustion.
		void LoadOggVorbisStream(std::string filename) throw (FileNotFound, std::bad_alloc);
        
        // Plays a sound effect identified by the parameter.  The parameter
        // should be one of the SoundIDs returned by LoadOggVorbis.
		void PlaySound(SoundID sound) throw ();
        
        // Starts playing the current music stream.  Does nothing if no stream
        // has been loaded.
		void PlayStream() throw ();
        
        // Pauses the current music stream.  Does nothing if no stream has been
        // loaded.
		void PauseStream() throw ();
        
        // Moves the current stream to time point 0.0.  Does nothing if no
        // stream has been loaded.
		void RewindStream() throw ();
        
        // Checks for completely played stream buffers and refills them if
        // necessary.  This should be called frequently, I call it every
        // frame.
		void ProcessStream() throw ();
        
        // Enable sound playback.  Sound playback is enabled by default.
		void Enable() throw ();
        
        // Disable sound playback.
		void Disable() throw ();
        
        // Flip the enabled/disabled state.
		void ToggleEnabled() throw ();
        
        // Delete all stored sound data.  After calling this function,
        // all existing SoundIDs will be invalid, and there will be
        // no stream loaded.
		void ClearSounds() throw ();
        
        // Delete a specific sound.  The parameter should be one of the SoundIDs
        // returned by LoadOggVorbis.
        void DeleteSound(SoundID sound) throw ();

        // This function is not available on most compilers.  As of this writing,
        // only Intel C++ and Comeau C++ support exported templates.  Remove
        // the #ifdef block if you want it.
#ifdef EXPORT_ENABLED
        // Delete an iterator range of sounds.
        export template <typename type>
        void DeleteSounds(type begin, type end);
#endif
	}
}

#endif
