the mac OpenAL-Soft.framework is compiled from the sources in the folder 'openal-soft'

it is git revision 4a1c0fedca9c15cf7a6f6949f64a58472a929be3
chosen because of the coreaudio backend was committed right before it

Changes to the revision:
 - add in config.h from running CMAKE
 - make sure to compile with -DAL_ALEXT_PROTOTYPES
 - link against Carbon and AudioUnit frameworks
