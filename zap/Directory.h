//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef __APPLE__
#error Directory.h is for Mac OS X only!
#endif

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>

namespace TNL
{
    template <typename> class Vector;
}

// This header is an interface for Objective-c++ calls on the Apple platform (see Directory.mm)
void moveToAppPath();
void prepareFirstLaunchMac();
void checkForUpdates();
void getAppResourcePath(std::string &fillPath);
void getApplicationSupportPath(std::string &fillPath);
void getDocumentsPath(std::string &fillPath);
void getBundlePath(std::string &fillPath);
void getExecutablePath(std::string &fillPath);

#endif

