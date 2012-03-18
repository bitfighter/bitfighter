/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2009 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifndef _SDL_config_minimal_linux_h
#define _SDL_config_minimal_linux_h

#include "SDL_platform.h"

/* This is a minimal configuration that can be used to build SDL on Linux */

#include <stdarg.h>

/* Enable the dummy audio driver (src/audio/dummy/\*.c) */
#define SDL_AUDIO_DRIVER_DUMMY	1

#define SDL_AUDIO_DRIVER_ALSA 1
#define SDL_AUDIO_DRIVER_DISK 1
#define SDL_AUDIO_DRIVER_OSS 1

/* Enable the stub cdrom driver (src/cdrom/dummy/\*.c) */
#define SDL_CDROM_DISABLED	1

/* Enable the joystick driver */
#define SDL_INPUT_LINUXEV 1
#define SDL_JOYSTICK_LINUX 1

/* Enable the shared object loader */
#define SDL_LOADSO_DLOPEN 1

/* Enable the thread support */
#define SDL_THREAD_PTHREAD 1
#define SDL_THREAD_PTHREAD_RECURSIVE_MUTEX 1

/* Enable the timer support */
#define SDL_TIMER_UNIX 1

/* Enable the video driver */
#define SDL_VIDEO_DRIVER_DUMMY	1
#define SDL_VIDEO_DRIVER_DGA 1
#define SDL_VIDEO_DRIVER_DUMMY 1
#define SDL_VIDEO_DRIVER_FBCON 1
#define SDL_VIDEO_DRIVER_X11 1
#define SDL_VIDEO_DRIVER_X11_DGAMOUSE 1
/*
#define SDL_VIDEO_DRIVER_X11_DYNAMIC "libX11.so"
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT "libXext.so"
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XRANDR "libXrandr.so"
#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XRENDER "libXrender.so"
*/
#define SDL_VIDEO_DRIVER_X11_VIDMODE 1
#define SDL_VIDEO_DRIVER_X11_XINERAMA 1
#define SDL_VIDEO_DRIVER_X11_XME 1
#define SDL_VIDEO_DRIVER_X11_XRANDR 1
#define SDL_VIDEO_DRIVER_X11_XV 1

/* Enable OpenGL */
#define SDL_VIDEO_OPENGL 1
#define SDL_VIDEO_OPENGL_GLX 1

/* Disable screensaver */
#define SDL_VIDEO_DISABLE_SCREENSAVER 1

/* Enable assembly routines */
#define SDL_ASSEMBLY_ROUTINES 1

/* Useful headers */
#define HAVE_ALLOCA_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_STDIO_H 1
#define STDC_HEADERS 1
#define HAVE_STDLIB_H 1
#define HAVE_STDARG_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_CTYPE_H 1
#define HAVE_MATH_H 1
#define HAVE_ICONV_H 1
#define HAVE_SIGNAL_H 1

/* C library functions */
#define HAVE_MALLOC 1
#define HAVE_CALLOC 1
#define HAVE_REALLOC 1
#define HAVE_FREE 1
#define HAVE_ALLOCA 1
#ifndef _WIN32 /* Don't use C runtime versions of these on Windows */
#define HAVE_GETENV 1
#define HAVE_PUTENV 1
#define HAVE_UNSETENV 1
#endif
#define HAVE_QSORT 1
#define HAVE_ABS 1
#define HAVE_BCOPY 1
#define HAVE_MEMSET 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMCMP 1
#define HAVE_STRLEN 1
/* #undef HAVE_STRLCPY */
/* #undef HAVE_STRLCAT */
#define HAVE_STRDUP 1
/* #undef HAVE__STRREV */
/* #undef HAVE__STRUPR */
/* #undef HAVE__STRLWR */
/* #undef HAVE_INDEX */
/* #undef HAVE_RINDEX */
#define HAVE_STRCHR 1
#define HAVE_STRRCHR 1
#define HAVE_STRSTR 1
/* #undef HAVE_ITOA */
/* #undef HAVE__LTOA */
/* #undef HAVE__UITOA */
/* #undef HAVE__ULTOA */
#define HAVE_STRTOL 1
#define HAVE_STRTOUL 1
/* #undef HAVE__I64TOA */
/* #undef HAVE__UI64TOA */
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_ATOI 1
#define HAVE_ATOF 1
#define HAVE_STRCMP 1
#define HAVE_STRNCMP 1
/* #undef HAVE__STRICMP */
#define HAVE_STRCASECMP 1
/* #undef HAVE__STRNICMP */
#define HAVE_STRNCASECMP 1
#define HAVE_SSCANF 1
#define HAVE_SNPRINTF 1
#define HAVE_VSNPRINTF 1
#define HAVE_ICONV 1
#define HAVE_SIGACTION 1
#define HAVE_SETJMP 1
#define HAVE_NANOSLEEP 1
/* #undef HAVE_CLOCK_GETTIME */
#define HAVE_GETPAGESIZE 1
#define HAVE_MPROTECT 1


#endif /* _SDL_config_minimal_linux_h */
