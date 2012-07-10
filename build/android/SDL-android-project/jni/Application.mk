# Force availabliity of run-time type information, as well as exceptions
APP_CPPFLAGS += -frtti -fexceptions

# This is the normal requirement for c++ stl
#APP_STL := stlport_static

# Use the GNU stl instead because of boost problems
APP_STL := gnustl_static

APP_ABI := armeabi