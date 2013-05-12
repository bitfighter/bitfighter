/* oglconsole -- gpl license here */


#ifdef WIN32
#  define _CRT_SECURE_NO_DEPRECATE    // Avoid warnings about fopen and vsnprintf
#endif 


#include "oglconsole.h"
#include "InputCodeEnum.h"

#ifndef BF_NO_CONSOLE

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

// Begin Bitfighter specific block
#ifndef ZAP_DEDICATED
#  ifdef TNL_OS_MOBILE
#     include "SDL_opengles.h"
#  else
#     include "SDL_opengl.h"
#  endif
#endif
// End Bitfighter specific block


// There are two fonts available: The original ConsoleFont and the alternate PackedFont,
// which is a chunkier, more pixelly font than ConsoleFont.
// Uncomment the following line to use PackedFont:
//#define OGLCONSOLE_USE_PACKED_FONT

#ifdef OGLCONSOLE_USE_PACKED_FONT

#define OGLCONSOLE_USE_ALPHA_TEXT

#  include "PackedFont.c"

#  define FIRST_CHARACTER '\x00'
#  define LAST_CHARACTER  '\x7F'
#  define CHAR_PIXEL_W 8
#  define CHAR_PIXEL_H 8
#  define CHAR_WIDTH (CHAR_PIXEL_W/128.0) /* ogl tex coords */
#  define CHAR_HEIGHT (CHAR_PIXEL_H/64.0) /* ogl tex coords */

#else

#  include "ConsoleFont.c"

#  define FIRST_CHARACTER ' '
#  define LAST_CHARACTER  '~'

#  define CHAR_PIXEL_W 6
#  define CHAR_PIXEL_H 13
#  define CHAR_WIDTH 0.0234375 /* ogl tex coords */
#  define CHAR_HEIGHT 0.203125 /* ogl tex coords */

#endif


#define CHAR_COLS (128/CHAR_PIXEL_W)
#define CHAR_ROWS (64/CHAR_PIXEL_H)

/* If we don't know how to retrieve the time then we can just use a number of
 * frames to divide up the time it takes to transition between "hidden" and
 * "visible" console visibility modes */
#define SLIDE_STEPS 10

#ifdef ZAP_DEDICATED
   typedef int GLuint;
   typedef double GLDouble;
   typedef double GLdouble;
   typedef int GLint;
#endif

GLuint OGLCONSOLE_glFontHandle = 0;
int OGLCONSOLE_CreateFont()
{
#ifndef ZAP_DEDICATED
    { int err = glGetError(); if(err) printf("GL ERROR: %i\n",err); }
#  ifdef DEBUG
    puts("Creating OGLCONSOLE font");
#  endif
   
    /* Destroy old texture if it exists */
    if(glIsTexture(OGLCONSOLE_glFontHandle))
       glDeleteTextures(1, &OGLCONSOLE_glFontHandle);

    /* Get a font index from OpenGL */
    glGenTextures(1, &OGLCONSOLE_glFontHandle);    /* Create 1 texture, store in glFontHandle */
    { int err = glGetError(); if(err )printf("glGenTextures() error: %i\n",err); }
    
    /* Select our font */
    glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);
    { int err = glGetError(); if(err) printf("glBindTexture() error: %i\n",err); }

    /* Set some parameters i guess */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


#  ifdef OGLCONSOLE_CREATE_PACKED_FONT
	{
		unsigned int x, y;
		unsigned char *buf = _alloca(OGLCONSOLE_FontData.height*OGLCONSOLE_FontData.width/8);
		unsigned char *dst = buf;
		FILE *file;
		for (y = 0; y < OGLCONSOLE_FontData.height; ++y)
		{
			for (x = 0; x < OGLCONSOLE_FontData.width; ++x)
			{
				if (OGLCONSOLE_FontData.pixel_data[(y*OGLCONSOLE_FontData.width+x)*OGLCONSOLE_FontData.bytes_per_pixel])
					*dst|=(1<<(x&7));
				else
					*dst&=~(1<<(x&7));
				if ((x&7) == 7)
					++dst;
			}
		}

		file = fopen("packedfont.c", "w");
		if (file != NULL)
		{
			fprintf(file, 
				"static const struct\n"
				"{\n"
				"\tunsigned int 	 width;\n"
				"\tunsigned int 	 height;\n"
				"\tunsigned char	 pixel_data[%d];\n"
				"}\n"
				"OGLCONSOLE_FontData =\n"
				"{\n"
				"\t%d, %d,\n",
				dst - buf,
				128, 64
				);
			//OutputDebugStringA("  \"");
			for (x = 0; x < dst - buf; ++x)
			{
				if ((x & 15) == 0)
					fputs("\t", file);
				fprintf(file, "0x%02x, ", buf[x]);
				if ((x & 15) == 15)
					fputs("\n", file);
			}
			//OutputDebugStringA("\"\n");
			if ((x & 15) != 0)
				fputs("\n", file);
			fputs("};\n", file);
		}
	}
#  endif




#  ifdef OGLCONSOLE_USE_PACKED_FONT

   // Untested fix from Sam:
#     ifdef OGLCONSOLE_USE_ALPHA_TEXT
#       define ALPHA_FACT 1
#     else
#       define ALPHA_FACT 3
#     endif
   
   {
		/* Unpack font data */

#if defined(__GNUC__) && __GNUC__ >= 4  // gcc 4.7.0 on mingw appears to have problems with _alloca
      unsigned char data[OGLCONSOLE_FontData.height * OGLCONSOLE_FontData.width * ALPHA_FACT];
#else
      unsigned char *data = (unsigned char *) _alloca(OGLCONSOLE_FontData.height * OGLCONSOLE_FontData.width * ALPHA_FACT);
#endif

		const unsigned char *src = OGLCONSOLE_FontData.pixel_data;
		unsigned char *dst = data;
		unsigned int y, x, b;

		for (y = 0; y < OGLCONSOLE_FontData.height; ++y)
		{
			for (x = 0; x < OGLCONSOLE_FontData.width; x+=8)
			{
				for (b = 0; b < 8; ++b)
				{
					if ((*src >> b) & 1)
					{
#     ifndef OGLCONSOLE_USE_ALPHA_TEXT
						*dst++ = 255;
						*dst++ = 255;
#     endif
						*dst++ = 255;
					}
					else
					{
#     ifndef OGLCONSOLE_USE_ALPHA_TEXT
						*dst++ = 0;
						*dst++ = 0;
#     endif
						*dst++ = 0;
					}
				}
				src++;
			}
		}

		/* Upload our font */
#     ifdef OGLCONSOLE_USE_ALPHA_TEXT
		glTexImage2D(
				GL_TEXTURE_2D, 0, GL_ALPHA,
				OGLCONSOLE_FontData.width, OGLCONSOLE_FontData.height, 0,
				GL_ALPHA, GL_UNSIGNED_BYTE, data);
#     else
		glTexImage2D(
				GL_TEXTURE_2D, 0, GL_RGB,
				OGLCONSOLE_FontData.width, OGLCONSOLE_FontData.height, 0,
				GL_RGB, GL_UNSIGNED_BYTE, data);
#     endif
	}
#  else
	/* Upload our font */
#     ifdef OGLCONSOLE_USE_ALPHA_TEXT
	glTexImage2D(
			GL_TEXTURE_2D, 0, GL_ALPHA,
			OGLCONSOLE_FontData.width, OGLCONSOLE_FontData.height, 0,
			GL_ALPHA, GL_UNSIGNED_BYTE, OGLCONSOLE_FontData.pixel_data);
#     else
	glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB,
			OGLCONSOLE_FontData.width, OGLCONSOLE_FontData.height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, OGLCONSOLE_FontData.pixel_data);
#     endif
#  endif

    { int err = glGetError(); if(err) printf("glTexImage2D() error: %i\n",err); }
    
#  ifdef DEBUG
    puts("Created  OGLCONSOLE font");
#  endif
#endif		// ZAP_DEDICATED
    return 1;
}

/* TODO: Expose these macros to the user? */

/* This is the longest command line that the user can enter TODO: Make dynamic
 * so that the user can enter any length line */
#define MAX_INPUT_LENGTH 256

/* This is the number of command line entries that the console will remember (so
 * that the user can use the up/down keys to see and easily re-execute his past
 * commands) */
#define MAX_HISTORY_COUNT 255

/* This is the default number of lines for the console to remember (that is to
 * say, the user can scroll up and down to see what has been printed to the
 * console in the past, and this is the number of those lines, plus the number
 * of lines shown on the screen at all times) */
#define DEFAULT_MAX_LINES 256

/* OGLCONSOLE console structure */
struct _OGLCONSOLE_Console
{
    GLdouble mvMatrix[16];
    int mvMatrixUse;

    GLdouble pMatrix[16];

    /* Screen+scrollback lines (console output) */
    char *lines;
    int maxLines, lineQueueIndex, lineScrollIndex;

    /* History scrollback (command input) */
    char history[MAX_HISTORY_COUNT][MAX_INPUT_LENGTH];
    int historyQueueIndex, historyScrollIndex;
    int maxHistoryIndex;      // Count of number of items actually in history; starts at 0, grows until history is full

    /* Current input line */
    char inputLine[MAX_INPUT_LENGTH];
    int inputCursorPos, inputLineLength;
    int drawCursor;           // Defaults to true, can be used to make cursor blink

    /* Rows and columns of text to display */
    int textWidth, textHeight;

    /* Current column of text where new text will be inserted */
    char *outputCursor;
    int outputNewline;

    /* Width and height of a single character for the GL */
    GLdouble characterWidth, characterHeight;
    
    /* Basic options */
    int visibility;

    /* Various callback functions defined by the user */
    void(*enterKeyCallback)(OGLCONSOLE_Console console, char *cmd);

};

/* To save code, I've gone with an imperative "modal" kind of interface */
_OGLCONSOLE_Console *programConsole = NULL;

/* This console is the console currently receiving user input */
_OGLCONSOLE_Console *userConsole = NULL;

/* Set the callback for a console */
void OGLCONSOLE_EnterKey(void(*cbfun)(OGLCONSOLE_Console console, char *cmd))
{
    programConsole->enterKeyCallback = cbfun;
}

static
void OGLCONSOLE_DefaultEnterKeyCallback(OGLCONSOLE_Console console, char *cmd)
{
    OGLCONSOLE_Output(console,
            "No enter key callback is registered for this console!\n");
}


void OGLCONSOLE_Resize(_OGLCONSOLE_Console *console)
{
#ifndef ZAP_DEDICATED
    GLint viewport[4];
    GLdouble screenWidth, screenHeight;
    int oldTextWidth = console->textWidth;
    char * oldLines = console->lines;     // Preserve console text

    /* Textual dimensions */
    glGetIntegerv(GL_VIEWPORT, viewport);    
    console->textWidth = viewport[2] / CHAR_PIXEL_W;
    console->textHeight = viewport[3] / CHAR_PIXEL_H;
    screenWidth = (GLdouble)viewport[2] / (GLdouble)CHAR_PIXEL_W;    // width in chars
    screenHeight = (GLdouble)viewport[3] / (GLdouble)CHAR_PIXEL_H;   // height in chars
    console->characterWidth = 1.0 / (int)screenWidth;
    console->characterHeight = 1.0 / (int)screenHeight;

    /* Different values have different meanings for xMatrixUse:
        0) Do not change the matrix before rendering
        1) Upload the console's matrix before rendering
        2) Multiply the console's matrix before rendering */

    /* Initialize its projection matrix */
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glGetDoublev(GL_PROJECTION_MATRIX, console->pMatrix);
    glPopMatrix();

    /* Initialize its modelview matrix */
    console->mvMatrixUse = 1;
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glScaled(console->textWidth, console->textHeight, 1);
    glGetDoublev(GL_MODELVIEW_MATRIX, console->mvMatrix);
    glPopMatrix();


   /* Screen and scrollback lines */


   /* This cursor points to what line the top of the console view is scrolled to */
   console->lineScrollIndex = console->maxLines - console->textHeight + 1;      // --> Scroll to end of window

   {
      /* This is the total number of screen lines in memory (start blank) */
      console->maxLines = DEFAULT_MAX_LINES;
      /* Allocate space for text */
      //if (console->lines)
      //    free(console->lines);
      console->lines = (char*)malloc(console->maxLines*(console->textWidth+1));
      /* Initialize to empty strings */
      memset(console->lines, 0, console->maxLines*(console->textWidth+1));
      /* This variable represents whether or not a newline has been left */
      console->outputNewline = 0;
      /* This cursor points to the X pos where console output is next destined */
      console->outputCursor = console->lines;
      /* This cursor points to what line console output is next destined for */
      console->lineQueueIndex = 0;
      /* This cursor points to what line the console view is scrolled to */
      console->lineScrollIndex = console->maxLines - console->textHeight + 1;


      // Restore console text
      if(oldLines)
      {
         int gLine;
         for (gLine = 0; gLine < DEFAULT_MAX_LINES; gLine++)
         {
            char * s = oldLines + (gLine * oldTextWidth);
            if(s[0] != 0) 
               OGLCONSOLE_Output((OGLCONSOLE_Console)console, "%s\n",s);  // <== this nutso cast needed by VC++
         }
         free(oldLines);
      }
   }
#endif
}
 


void OGLCONSOLE_Reshape()
{
   OGLCONSOLE_Resize(userConsole);
}


OGLCONSOLE_Console OGLCONSOLE_Create()
{
    _OGLCONSOLE_Console *console;
    
#ifndef ZAP_DEDICATED
    OGLCONSOLE_CreateFont();
#endif

    /* Allocate memory for our console */
    console = (_OGLCONSOLE_Console*)malloc(sizeof(_OGLCONSOLE_Console));

    if(console == NULL)     // Malloc failed?
       return NULL;

	/* Size the console */
	console->lines = NULL;
	OGLCONSOLE_Resize(console);

    /* Screen and scrollback lines */
    /* This is the total number of screen lines in memory (start blank) */
    console->maxLines = DEFAULT_MAX_LINES;
    /* Allocate space for text */
	 if(console->lines)
	  	  free(console->lines);
    console->lines = (char*)malloc(console->maxLines*(MAX_INPUT_LENGTH+1));    // tw based on vp

    if(console->lines == NULL)
    {
       free(console);
       return NULL;
    }

    /* Initialize to empty strings */
    memset(console->lines, 0, console->maxLines*(MAX_INPUT_LENGTH+1));
    /* This variable represents whether or not a newline has been left */
    console->outputNewline = 0;
    /* This cursor points to the X pos where console output is next destined */
    console->outputCursor = console->lines;
    /* This cursor points to what line console output is next destined for */
    console->lineQueueIndex = 0;


    /* Initialize the user's input (command line) */
    console->inputLineLength = 0;
    console->inputCursorPos = 0;
    console->drawCursor = 1;     // On by default
    console->inputLine[0] = '\0';

    /* History lines */
    memset(console->history, 0, MAX_INPUT_LENGTH * MAX_HISTORY_COUNT);
    console->historyQueueIndex = 0;
    console->historyScrollIndex = -1;
    console->maxHistoryIndex = 0;

    /* Callbacks */
    console->enterKeyCallback = OGLCONSOLE_DefaultEnterKeyCallback;

    /* The console starts life invisible */
    console->visibility = 0;

    /* If no consoles existed before, we select this one for convenience */
    if (!programConsole) programConsole = console;
    if (!userConsole) userConsole = console;




    /* Temporary shit */
    OGLCONSOLE_Output(console, "Console initialized\n");

    OGLCONSOLE_Output(console,
            "Console display lines: %i\n", console->textHeight);

    OGLCONSOLE_Output(console,
            "Console display columns: %i\n", console->textWidth);

    OGLCONSOLE_Output(console,
            "Console input length: %i\n", MAX_INPUT_LENGTH);


    /* Return the opaque pointer to the programmer */
    return (OGLCONSOLE_Console)console;
}

/* This functoin is only used internally; the user ultimately invokes this
 * function through either a call to Destroy() or Quit(); the purpose of this
 * mechanism is to warn the user if he has explicitly destroyed a console that
 * was engaged in operation at the time they destroyed it (the only two
 * operations a console can be engaged in are receiving programmer interaction,
 * or receiving end-user interaction. fyi, "user" always refers to the
 * programmer, end-user refers to the real end-user) */
static void OGLCONSOLE_DestroyReal(OGLCONSOLE_Console C, int safe)
{
	if (C)
	{
		free(C->lines);
	}
    free(C);

    if (safe)
    {
        if (programConsole == C)
        {
            fprintf(stderr, 
            "Warning: OGLCONSOLE you just destroyed the programConsole!\n");
            programConsole = NULL;
        }

        if (userConsole == C)
        {
            fprintf(stderr,
            "Warning: OGLCONSOLE you just destroyed the userConsole!\n");
            userConsole = NULL;
        }
    }
}

/* The user can call this function to free a console. A warning is printed to
 * stderr if the user has destroyed a console destroyed while it was receiving
 * input, see DestroyReal() for details TODO: there are currently no semantics
 * under consideration for explicitly destroying an 'engaged' console WITHOUT
 * having a warning issued, nor is there a way to deselect a console without
 * also selecting a new one as of yet */
void OGLCONSOLE_Destroy(OGLCONSOLE_Console console)
{
    OGLCONSOLE_DestroyReal(console, 1);
}

/* This function frees all of the consoles that the library is actively aware
 * of, and is intended to be used by programs that have only a single console;
 * technically this particular function will free up to two consoles, but don't
 * count on it because that may change; no warnings are issued by this function */
void OGLCONSOLE_Quit()
{
    if (programConsole)
        OGLCONSOLE_DestroyReal(programConsole, 0);

    if (programConsole != userConsole && userConsole)
        OGLCONSOLE_DestroyReal(userConsole, 0);
    programConsole = NULL;
    userConsole = NULL;
}


/* Show or hide a console */
void OGLCONSOLE_SetVisibility(int visible)
{
    programConsole->visibility = visible != 0;
}

int OGLCONSOLE_GetVisibility()
{
	return userConsole->visibility;
}

/* This routine is meant for applications with a single console, if you use
 * multiple consoles in your program, use Render() instead */
void OGLCONSOLE_Draw() { OGLCONSOLE_Render(userConsole); }

void OGLCONSOLE_setCursor(int drawCursor) { userConsole->drawCursor = drawCursor; }

/* Internal functions for drawing text. You don't want these, do you? */
void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h,
        double z);
void OGLCONSOLE_DrawWrapString(char *s, double x, double y, double w, double h,
        double z, int wrap);
void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h,
        double z);

/* This function draws a single specific console; if you only use one console in
 * your program, use Draw() instead */
void OGLCONSOLE_Render(OGLCONSOLE_Console C)
{
#ifndef ZAP_DEDICATED

    /* Don't render hidden console */
    if(C->visibility == 0) return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixd(C->pMatrix);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(C->mvMatrix);

    glPushAttrib(GL_ALL_ATTRIB_BITS);

/*    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);*/

    /* TODO: This SHOULD become an option at some point because the
     * infrastructure for "real" consoles in the game (like you could walk up to
     * a computer terminal and manipulate a console on a computer using
     * oglconsole) already exists; you'd want depth testing in that case */
    glDisable(GL_DEPTH_TEST);

	/* ADDED */
	glDisable( GL_POLYGON_SMOOTH );

    /* Render hiding / showing console in a special manner. Zero means hidden. 1
     * means visible. All other values are traveling toward zero or one. TODO:
     * Make this time dependent */
    if (C->visibility != 1)
    {
        double d; /* bra size */
        int v = C->visibility;

        /* Count down in both directions */
        if (v < 0)
        {
            v ^= -1;
            C->visibility++;
        }
        else
        {
            v = SLIDE_STEPS - v;
            C->visibility--;
        }

        d = C->textHeight * C->characterHeight * (1.0 - v * (1.0 / SLIDE_STEPS));
        glTranslated(0, d, 0);
//d = 0.04 * v;
//glTranslated(0, 1-d, 0);
    }

    /* First we draw our console's background TODO: Add something fancy? */
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(.1,0,0, 0.75);

    glBegin(GL_QUADS);
    glVertex3d(-1,0,0);      // Draw from -1 to 2 to ensure complete screen coverage... totally hacky, but works!
    glVertex3d(2,0,0);
    glVertex3d(2,2,0);
    glVertex3d(-1,2,0);
    glEnd();

#ifndef OGLCONSOLE_USE_ALPHA_TEXT
    // Change blend mode for drawing text
    glBlendFunc(GL_ONE, GL_ONE);
#endif

    /* Select the console font */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, OGLCONSOLE_glFontHandle);

    /* Recolor text */
    glColor3d(0,1,0);

    /* Render console contents */
    glBegin(GL_QUADS);
    {
        /* Graphical line, and line in lines[] */
        int gLine, tLine = C->lineScrollIndex;

        /* Iterate through each line being displayed */
        for (gLine = 0; gLine < C->textHeight; gLine++)
        {
            /* Draw this line of text adjusting for user scrolling up/down */
            OGLCONSOLE_DrawString(C->lines + (tLine * C->textWidth),
                    0,
                    (C->textHeight - gLine) * C->characterHeight,
                    C->characterWidth,
                    C->characterHeight,
                    0);

            /* Grab next line of text using wheel-queue wrapping */
            if (++tLine >= C->maxLines) tLine = 0;
        }

        /* Here we draw the current commandline, it will either be a line from
         * the command history or the line being edited atm */
        if (C->historyScrollIndex >= 0)
        {
            glColor3d(1,0,0);    // red
            OGLCONSOLE_DrawString(
                    C->history[C->historyScrollIndex],
                    0, 0,
                    C->characterWidth,
                    C->characterHeight,
                    0);
        }
        else
        {
            /* Draw input line cyan */
            glColor3d(0,1,1);
            OGLCONSOLE_DrawString(C->inputLine,
                    0, 0,
                    C->characterWidth,
                    C->characterHeight,
                    0);
        }

        /* Draw cursor */
        //glColor3d(1,1,.5);    // <== use whatever color we drew the line with
        if(C->drawCursor)     
           OGLCONSOLE_DrawCharacter('_'-FIRST_CHARACTER,
                    C->inputCursorPos * C->characterWidth, 0,
                    C->characterWidth,
                    C->characterHeight,
                    0);
    }
    glEnd();

    /* Relinquish our rendering settings */
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPopAttrib();
#endif
}

/* Issue rendering commands for a single a string */
void OGLCONSOLE_DrawString(char *s, double x, double y, double w, double h,
        double z)
{
    while (*s)
    {
        OGLCONSOLE_DrawCharacter(*s-FIRST_CHARACTER, x, y, w, h, z);
        s++;
        x += w;
    }
}

/* Issue rendering commands for a single a string */
void OGLCONSOLE_DrawWrapString(char *s, double x, double y, double w, double h,
        double z, int wrap)
{
    int pos = 0;
    double X = x;

    while (*s)
    {
        OGLCONSOLE_DrawCharacter(*s-FIRST_CHARACTER, X, y, w, h, z);
        s++;
        X += w;

        if (++pos >= wrap)
        {
            pos = 0;
            y += h;
            X = x;
        }
    }
}

/* Issue rendering commands for a single character */
void OGLCONSOLE_DrawCharacter(int c, double x, double y, double w, double h,
        double z)
{
//  static int message = 0;
    double cx, cy, cX, cY;
    
//    if (c < FIRST_CHARACTER || c > LAST_CHARACTER)
//        c = (c - FIRST_CHARACTER) % (LAST_CHARACTER - FIRST_CHARACTER);
//    else c -= FIRST_CHARACTER;


   
#ifdef OGLCONSOLE_USE_PACKED_FONT
    cx = (c % CHAR_COLS) * CHAR_WIDTH;
    cy = (c / CHAR_COLS) * CHAR_HEIGHT + CHAR_HEIGHT;
    cX = cx + CHAR_WIDTH;
    cY = cy - CHAR_HEIGHT;
#else
    cx = (c % 42) * CHAR_WIDTH;
    cy = 1.0 - (c / 42) * CHAR_HEIGHT;
    cX = cx + CHAR_WIDTH;
    cY = 1.0 - (c / 42 + 1) * CHAR_HEIGHT;
#endif



/*  if (message != c)
    {
        printf("For %i we got %f, %f\n", c, x, y);
        message = c;
    }*/

#ifndef ZAP_DEDICATED
    /* This should occur outside of this function for optimiation TODO: MOVE IT */
    glTexCoord2d(cx, cy); glVertex3d(x,   y,   z);
    glTexCoord2d(cX, cy); glVertex3d(x+w, y,   z);
    glTexCoord2d(cX, cY); glVertex3d(x+w, y+h, z);
    glTexCoord2d(cx, cY); glVertex3d(x,   y+h, z);
#endif
}

/* This is the final, internal function for printing text to a console */
void OGLCONSOLE_Output(OGLCONSOLE_Console C, const char *s, ...)
{
   if(C)
   {
     va_list argument;
     /* cache some console properties */
     int lineQueueIndex = C->lineQueueIndex;
     int lineScrollIndex = C->lineScrollIndex;
     int textWidth = C->textWidth;
     int maxLines = C->maxLines;
 
     /* String buffer */
     char output[MAX_CONSOLE_OUTPUT_LENGTH];
 
     /* string copy cursors */
     char *consoleCursor, *outputCursor = output;
 
     /* Acrue arguments in argument list */
     va_start(argument, s);
     vsnprintf(output, MAX_CONSOLE_OUTPUT_LENGTH, s, argument);
     va_end(argument);
 
 
 
     /* This cursor tells us where in the console display we are currently
      * copying text into from the "output" string */
     consoleCursor = C->outputCursor;
 
     while (*outputCursor)
     {
         /* Here we check to see if any conditions require console line
          * advancement. These two conditions are:
             1) Hitting the end of the screen
             2) Getting a newline character (indicated by "outputNewline") */
         if((C->outputNewline) ||
             (consoleCursor - (C->lines + lineQueueIndex * textWidth))
                 >= (textWidth - 1))
         {
             C->outputNewline = 0;
 
             //puts("incrementing to the next line");
 
             /* Inrement text-line index, with wrapping */
             if (++lineQueueIndex >= maxLines)
                 lineQueueIndex = 0;
 
             /* Scroll the console display one line TODO: Don't scroll if the console is
              * currently scrolled away from the end of output? */
             if (++lineScrollIndex >= maxLines)
                 lineScrollIndex = 0;
 
             /* Reposition the cursor at the beginning of the new line */
             consoleCursor = C->lines + lineQueueIndex * C->textWidth;
         }
         
         /* \r seems to make the console print some kind of random garbage.  So 
          * let's not print it! */
         if(*outputCursor == '\r') { outputCursor++; continue; }    


         /* If we encounter a newline character, we set the newline flag, which
          * tells the console to advance one line before it prints the next
          * character. The reason we do it this way is to defer line-advancement,
          * and thus we needn't suffer through a needless blank line between
          * console output and the command line, wasting precious screen
          * real-estate */
         if (*outputCursor == '\n')
         {
             C->outputNewline = 1;
             outputCursor++;
             continue;
         }
 
         /* If we encounter a tab character we must expand that character
          * appropriately */
         if (*outputCursor == '\t')
         {
             const int TAB_WIDTH = 8;
 
             int n = consoleCursor - (C->lines + lineQueueIndex * textWidth);
             //printf("column: %i\n", n);
 
             /* Are we indenting our way off the edge of the screen? */
             if (textWidth - n <= TAB_WIDTH)
             {
                 /* Switch on the console's newline bit, and advance through the
                  * string output we've been given */
                 C->outputNewline = 1;
                 outputCursor++;
                 continue;
             }
 
             /* Normal indent */
             else
             {
                 n = TAB_WIDTH - n % TAB_WIDTH;
                 while (n--) *(consoleCursor++) = ' ';
                 outputCursor++;
                 continue;
             }
         }
 
         /* copy a single character */
         *(consoleCursor++) = *(outputCursor++);
     }
 
     /* Unless we're at the very end of our current line, we finish up by capping
      * a NULL terminator on the current line */
     if (consoleCursor != C->lines + (lineQueueIndex+1) *C->textWidth -1)
         *consoleCursor = '\0';
 
     /* Restore cached values */
     C->lineQueueIndex = lineQueueIndex;
     C->lineScrollIndex = lineScrollIndex;
     C->outputCursor = consoleCursor; // TODO: confusing variable names
 
     /* old way of copying the text into the console */
     //strcpy(C->lines[C->lineQueueIndex], output);
#ifdef DEBUG
    printf("Copied \"%s\" into line %i\n", output, C->lineQueueIndex);
#endif
   }
}



/* Internal encapsulation of the act for adding a command the user executed to
 * their command history for that particular console */
void OGLCONSOLE_AddHistory(OGLCONSOLE_Console C, char *s)
{
    /* Skip blank lines */
    int blank = 1;
    int i = 0;
    while (s[i])
    {
       if(s[i] != ' ' && s[i] != '\t')
       {
          blank = 0;
          break;
       }
       i++;
    }

    if(blank) 
       return;

   strcpy(C->history[C->historyQueueIndex], s);      // strcpy(dest, src)

    C->maxHistoryIndex++;

    if (C->maxHistoryIndex >= MAX_HISTORY_COUNT)
        C->maxHistoryIndex = MAX_HISTORY_COUNT;


    C->historyQueueIndex++;

    if (C->historyQueueIndex >= MAX_HISTORY_COUNT)
        C->historyQueueIndex = 0;
}

void OGLCONSOLE_YankHistory(_OGLCONSOLE_Console *console)
{
    /* First we have to see if we are browsing our command history */
    if (console->historyScrollIndex != -1)
    {
        /* Copy the selected command into the current input line */
        strcpy(console->inputLine,
                console->history[console->historyScrollIndex]);

        /* Set up this shite */
        console->inputCursorPos  = 
            console->inputLineLength =
            strlen(console->inputLine);

        /* Drop out of history browsing mode */
        console->historyScrollIndex = -1;
    }
}

void OGLCONSOLE_ShowConsole()
{
   userConsole->visibility += SLIDE_STEPS;
   if( userConsole->visibility > SLIDE_STEPS)
      userConsole->visibility = SLIDE_STEPS;
}


void OGLCONSOLE_HideConsole()
{
   userConsole->visibility -= SLIDE_STEPS;

   if( userConsole->visibility < -SLIDE_STEPS)
      userConsole->visibility = -SLIDE_STEPS;
}  


#define KEY_RETURN      Zap::KEY_ENTER
#define KMOD_SHIFT      1

#define SHOW_CONSOLE_KEY Zap::KEY_NONE      // Bitfighter console created via different mechanism
#define HIDE_CONSOLE_KEY Zap::KEY_ESCAPE


int wrap(_OGLCONSOLE_Console *userConsole, int index)
{
   if(index < 0)
      return userConsole->maxHistoryIndex - 1;
   else if(index >= userConsole->maxHistoryIndex)
      return 0;
   else 
      return index;
}


int getCurrentLineLength(_OGLCONSOLE_Console *userConsole)
{
   if(userConsole->historyScrollIndex == -1)
      return userConsole->inputLineLength;
   else
      return strlen(userConsole->history[userConsole->historyScrollIndex]);
}


void putCursorAtEndOfLine(_OGLCONSOLE_Console *userConsole)
{
   userConsole->inputCursorPos = getCurrentLineLength(userConsole);
}


int OGLCONSOLE_KeyEvent(int sym, int mod)
{
    /* If the terminal is hidden we only check for show/hide key */
    if (userConsole->visibility < 1)
    {
        if (sym == SHOW_CONSOLE_KEY)
        {  
            OGLCONSOLE_ShowConsole();
            return 1;
        }
        
        return 0;
    }

    /* Check for hide key */
    if (sym == HIDE_CONSOLE_KEY)
    {
        /* Tell console to slide into closing */
        OGLCONSOLE_HideConsole();

        return 1;
    }

   /* The operation for delete and backspace keys are very similar */
    else if (sym == Zap::KEY_DELETE
         ||  sym == Zap::KEY_BACKSPACE)
    {
        /* If string is not empty */
        if (getCurrentLineLength(userConsole))
        {
            char *end, *c;

            /* Yank the command history if necessary */
            OGLCONSOLE_YankHistory(userConsole);

            /* Is this a backspace? */
            if (sym == Zap::KEY_BACKSPACE)
            {
               /* Backspace operations bail if the cursor is at the beginning
                 * of the input line */
                if (userConsole->inputCursorPos == 0)
                    return 1;

                /* This is all that differentiates the backspace from the delete
                 * key */
                userConsole->inputCursorPos--;
            }

            /* Delete key operations bail if the cursor is at the end of the
             * input line */
            else if (userConsole->inputCursorPos == userConsole->inputLineLength)
                return 1;

            /* Last we shift affected text to the left, overlapping the
             * erased character */
            c   = userConsole->inputLine +   userConsole->inputCursorPos;
            end = userConsole->inputLine + --userConsole->inputLineLength;

            while (c <= end)
            {
                *c = *(c+1);
                c++;
            }
        }

        return 1;
    }

    else if (sym == KEY_RETURN)
    {
         /* Yank the command history if necessary */
         OGLCONSOLE_YankHistory(userConsole);

         /* Add user's command to history */
         OGLCONSOLE_AddHistory(userConsole, userConsole->inputLine);

         /* Print user's command to the console */
         OGLCONSOLE_Output(userConsole, "%s\n", userConsole->inputLine);

         /* Invoke console's enter-key callback function */
         userConsole->enterKeyCallback(userConsole,userConsole->inputLine);

         /* Erase command line */
         userConsole->inputCursorPos = 0;
         userConsole->inputLineLength = 0;
         userConsole->inputLine[0] = '\0';

         return 1;
    }

    // Page up key
    else if (sym == Zap::KEY_PAGEUP)
    {
        userConsole->lineScrollIndex -= userConsole->textHeight / 2;

        if (userConsole->lineScrollIndex < 0)
            userConsole->lineScrollIndex += userConsole->maxLines;

        return 1;
    }

    // Page down key
    else if (sym == Zap::KEY_PAGEDOWN)
    {
        userConsole->lineScrollIndex += userConsole->textHeight / 2;

        if (userConsole->lineScrollIndex >= userConsole->maxLines)
            userConsole->lineScrollIndex -= userConsole->maxLines;

        return 1;
    }
     // Home key
     else if (sym == Zap::KEY_HOME)
     {
         /* Yank the command history if necessary */
         OGLCONSOLE_YankHistory(userConsole);

         userConsole->inputCursorPos = 0;
         return 1;
     }

     // End key
     else if (sym == Zap::KEY_END)
     {
         /* Yank the command history if necessary */
         OGLCONSOLE_YankHistory(userConsole);

         putCursorAtEndOfLine(userConsole);
         return 1;
     }


    // Arrow key up
    else if (sym == Zap::KEY_UP)
    {
        // Shift key is for scrolling the output display
        if (mod & KMOD_SHIFT)
        {
            if (--userConsole->lineScrollIndex < 0)
                userConsole->lineScrollIndex = userConsole->maxLines-1;
        }

        // No shift key is for scrolling through command history
        else
        {
            // -1 means we aren't looking at history yet
            if (userConsole->historyScrollIndex == -1)
            {
                userConsole->historyScrollIndex = wrap(userConsole, userConsole->historyQueueIndex - 1);
                putCursorAtEndOfLine(userConsole);
            }
            else     // userConsole->historyScrollIndex > -1
            {
               // If user hits up when they are already at the first index, do nothing
               // otherwise progress to previous history item
               if(userConsole->historyScrollIndex != userConsole->historyQueueIndex)
               {
                   userConsole->historyScrollIndex = userConsole->historyScrollIndex - 1 < 0 ? 0 : userConsole->historyScrollIndex - 1;
                   putCursorAtEndOfLine(userConsole);
               }
            }
        }

        return 1;
    }

    // Arrow key down
    else if (sym == Zap::KEY_DOWN)
    {
        // Shift key is for scrolling the output display
        if (mod & KMOD_SHIFT)
        {
            if (++userConsole->lineScrollIndex >= userConsole->maxLines)
            {
                userConsole->lineScrollIndex = 0;
                putCursorAtEndOfLine(userConsole);
            }
        }

        // No shift key is for scrolling through command history
        else
        {
            // -1 means we aren't look at history yet
            if (userConsole->historyScrollIndex != -1)
            {
                // Wrap our history scrolling
               userConsole->historyScrollIndex = wrap(userConsole, userConsole->historyScrollIndex + 1);

                // If we've returned to our current position in the command
                // history, we'll just drop out of history mode
                if (userConsole->historyScrollIndex == 
                           (userConsole->maxHistoryIndex == MAX_HISTORY_COUNT ? userConsole->historyQueueIndex : 0))
                     userConsole->historyScrollIndex = -1;

                putCursorAtEndOfLine(userConsole);
            }
        }
        return 1;
    }

    // Arrow key left
    else if (sym == Zap::KEY_LEFT)
    {
        /* Yank the command history if necessary */
        OGLCONSOLE_YankHistory(userConsole);

        if (userConsole->inputCursorPos > 0)
            userConsole->inputCursorPos--;

        return 1;
    }

    // Arrow key right
    else if (sym == Zap::KEY_RIGHT)
    {
        /* Yank the command history if necessary */
        OGLCONSOLE_YankHistory(userConsole);

        if (userConsole->inputCursorPos <
            userConsole->inputLineLength)
            userConsole->inputCursorPos++;

        return 1;
    }

    return 0;
}

int OGLCONSOLE_CharEvent(int unicode)
{
    /* If the terminal is hidden, no need to process input */
    if (userConsole->visibility < 1)
    {
        return 0;
    }

    if (unicode == 0)
    {
       return 0;
    }

   /* Only insert the char if there is room */
   if (userConsole->inputLineLength < MAX_INPUT_LENGTH - 1)
   {
      char *c, *d;

      /* Yank the command history if necessary */
      OGLCONSOLE_YankHistory(userConsole);

      /* Point to the cursor position and the end of the string */
      c = userConsole->inputLine + userConsole->inputCursorPos;
      d = userConsole->inputLine + userConsole->inputLineLength + 1;

      /* Slide some of the string to the right */
      for (; d != c; d--)
         *d = *(d-1);

      /* Insert new character */
      *c = (char)unicode;

      /* Increment input line length counter */
      userConsole->inputLineLength++;

      /* Advance input cursor position */
      userConsole->inputCursorPos++;
   }

   return 1;
}

#endif // BF_NO_CONSOLE
