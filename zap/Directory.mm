//-----------------------------------------------------------------------------------
//
// Bitfighter - A multiplayer vector graphics space game
// Based on Zap demo released for Torque Network Library by GarageGames.com
//
// Derivative work copyright (C) 2008-2009 Chris Eykamp
// Original work copyright (C) 2004 GarageGames.com, Inc.
// Other code copyright as noted
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful (and fun!),
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#ifndef __APPLE__
#error Directory.mm is for Mac OS X only!
#endif

#include "Directory.h"
#include "tnlVector.h"
#ifdef TNL_OS_MAC_OSX
#import <Cocoa/Cocoa.h>
#import "SUUpdater.h"
#define SPARKLE_APPCAST_URL @"http://bitfighter.org/files/getDownloadUrl.php"
#else
#import <Foundation/Foundation.h>
#endif

using TNL::Vector;
using std::string;


void moveToAppPath()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSFileManager *fm = [NSFileManager defaultManager];
    
    //On load, change to the application directory so we can get to the graphics/sounds/etc...
    [fm changeCurrentDirectoryPath:[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent]];
    
    [pool release];
}


void prepareFirstLaunchMac()
{
#ifdef TNL_OS_MAC_OSX
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSFileManager *fm = [NSFileManager defaultManager];

    NSArray *appSupportPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    NSString *fullAppSupportPath = [NSString stringWithFormat:@"%@/%@", [appSupportPaths objectAtIndex:0], bundleName];
        
    //Link preferences
    NSArray *documentsPath = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *prefencesPath = [[documentsPath objectAtIndex:0] stringByAppendingPathComponent:@"Bitfighter Settings"];
    if ([fm respondsToSelector:@selector(createSymbolicLinkAtPath:withDestinationPath:error:)])
        [fm createSymbolicLinkAtPath:prefencesPath withDestinationPath:fullAppSupportPath error:NULL];
    else
        [fm createSymbolicLinkAtPath:prefencesPath pathContent:fullAppSupportPath];

    [pool release];
#endif
}


void checkForUpdates()
{
#ifdef TNL_OS_MAC_OSX
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    SUUpdater* updater = [SUUpdater sharedUpdater];
    [updater setFeedURL:[NSURL URLWithString:SPARKLE_APPCAST_URL]];
    [updater setSendsSystemProfile:YES];
    [updater checkForUpdatesInBackground];
    [pool release];
#endif
}


// Used for setting -rootdatadir; corresponds to the location from which most 
// resources will be loaded
void getUserDataPath(std::string &fillPath)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
#ifdef TNL_OS_MAC_OSX
    // OSX used the Application Support directory
    NSArray *appSupportPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    fillPath = std::string([[NSString stringWithFormat:@"%@/%@", [appSupportPaths objectAtIndex:0], bundleName] UTF8String]);
#else // TNL_OS_IOS
    // iOS uses the resources straight from the bundle
    getAppResourcePath(fillPath);  
#endif
    
    [pool release];
}


// Used for setting -sfxdir; corresponds to the path of the app's bundled resources
void getAppResourcePath(std::string &fillPath)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
#ifdef TNL_OS_MAC_OSX
    fillPath = std::string([[NSString stringWithFormat:@"%@",[[NSBundle mainBundle] resourcePath]] UTF8String]);
#else // TNL_OS_IOS
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    fillPath = std::string([resourcePath UTF8String]);
#endif
    
    [pool release];
}


// Used for setting -inidir
void getDocumentsPath(std::string &fillPath)
{
   // Only needed for iOS (we need some read/write directory)
#ifdef TNL_OS_IOS
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSArray *documentPaths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    fillPath = std::string([[documentPaths objectAtIndex:0] UTF8String]);
     
    [pool release];
#endif
}
