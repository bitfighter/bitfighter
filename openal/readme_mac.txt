the mac OpenAL-Soft.framework is compiled from the sources in the folder 'openal-soft'

checked out git revision 1b773a858534693056161c90702c4cdb013e8a64

Changes to the revision:
 - add in config.h from running CMAKE
 - make sure to compile with -DAL_ALEXT_PROTOTYPES
 - link against Carbon, CoreAudio, AudioUnit, AudioToolkit frameworks
