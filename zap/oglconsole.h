/* oglconsole -- gpl license here */

#ifndef _OGLCONSOLE_H
#define _OGLCONSOLE_H

#if defined(ZAP_DEDICATED) || defined(TNL_OS_MOBILE)

struct OGLCONSOLE_Console
{
	// Do nothing
};

// define some empty function that will do nothing
#define OGLCONSOLE_Output
#define OGLCONSOLE_Print
#define OGLCONSOLE_HideConsole()
#define OGLCONSOLE_ShowConsole()
#define OGLCONSOLE_EnterKey
#define OGLCONSOLE_setCursor
#define OGLCONSOLE_Draw()
#define OGLCONSOLE_Reshape()
#define OGLCONSOLE_ProcessBitfighterTextInputEvent
#define OGLCONSOLE_ProcessBitfighterKeyEvent
#define OGLCONSOLE_GetVisibility() false
#define OGLCONSOLE_CreateFont()
#define OGLCONSOLE_Quit()
#define OGLCONSOLE_Create() OGLCONSOLE_Console()

#else

/* Opaque to you you lowly user */
typedef struct _OGLCONSOLE_Console *OGLCONSOLE_Console;



#ifdef __cplusplus
extern "C" {
#endif



/* Initialize/uninitialize OGLConsole */
OGLCONSOLE_Console OGLCONSOLE_Create();
void OGLCONSOLE_Destroy(OGLCONSOLE_Console console);
void OGLCONSOLE_Quit();

// These functions munge the Bitfighter keycodes into something oglconsole can understand
int OGLCONSOLE_ProcessBitfighterKeyEvent(int inputCode);    // Returns true if console is open, false if it has been closed
int OGLCONSOLE_ProcessBitfighterTextInputEvent(char ascii);    // Returns true if console is open, false if it has been closed

void OGLCONSOLE_ShowConsole();
void OGLCONSOLE_HideConsole();

/* Set console which has PROGRAMMER focus (not application focus) */

/* This function renders the console */
void OGLCONSOLE_Draw();
void OGLCONSOLE_Render(OGLCONSOLE_Console console);

/* Set whether cursor should be visible -- can be used to make cursor blink */
void OGLCONSOLE_setCursor(int drawCursor);      

/* Handle resize window events */
void OGLCONSOLE_Reshape();

/* Print to the console */
void OGLCONSOLE_Print(const char *s, ...);
void OGLCONSOLE_Output(OGLCONSOLE_Console console, const char *s, ...);

/* Register a callback with the console */
void OGLCONSOLE_EnterKey(void(*cbfun)(OGLCONSOLE_Console console, char *cmd));

/* This function tries to handle the incoming keydown event. In the future there may
 * be non-SDL analogs for input systems such as GLUT. Returns true if the event
 * was handled by the console. If console is hidden, no events are handled. */
int OGLCONSOLE_KeyEvent(int key, int mod);
int OGLCONSOLE_CharEvent(int unicode);

/* Sets the current console for receiving user input */
void OGLCONSOLE_FocusConsole(OGLCONSOLE_Console console);

/* Sets the current console for making options changes to */
void OGLCONSOLE_EditConsole(OGLCONSOLE_Console console);

/* Sets the dimensions of the console in lines and columns of characters. */
void OGLCONSOLE_SetDimensions(int width, int height);

/* Use this if you want to populate console command history yourself */
void OGLCONSOLE_AddHistory(OGLCONSOLE_Console console, char *s);

/* Show or hide the console */
void OGLCONSOLE_SetVisibility(int visible);
int OGLCONSOLE_GetVisibility();

/* Create the console font */
int OGLCONSOLE_CreateFont();

#ifdef __cplusplus
}
#endif
#endif

#endif

