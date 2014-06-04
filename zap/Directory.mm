//------------------------------------------------------------------------------
// Copyright Chris Eykamp
// See LICENSE.txt for full copyright information
//------------------------------------------------------------------------------

#ifndef __APPLE__
#error Directory.mm is for Mac OS X only!
#endif

#include "Directory.h"
#include "tnlVector.h"
#ifdef TNL_OS_MAC_OSX
#import <Cocoa/Cocoa.h>
#import "SUUpdater.h"
// Here we add a GET parameter if we're running on older OSX.  This way we
// can serve up a different download URL
#ifdef __x86_64__
#define SPARKLE_APPCAST_URL @"http://bitfighter.org/files/getDownloadUrl.php"
#else
#define SPARKLE_APPCAST_URL @"http://bitfighter.org/files/getDownloadUrl.php?legacy=1"
#endif
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
    [updater checkForUpdatesInBackground];
    [pool release];
#endif
}


// Used for setting -rootdatadir; corresponds to the location from which most 
// resources will be loaded
void getApplicationSupportPath(std::string &fillPath)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    // OSX used the Application Support directory
    NSArray *appSupportPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    fillPath = std::string([[NSString stringWithFormat:@"%@", [appSupportPaths objectAtIndex:0]] UTF8String]);
    
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


void getBundlePath(std::string &fillPath)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSString *bundlePath = [[NSBundle mainBundle] bundlePath];
    fillPath = std::string([bundlePath UTF8String]);
    
    [pool release];
}


void getExecutablePath(std::string &fillPath)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    NSString *executablePath = [[NSBundle mainBundle] executablePath];
    fillPath = std::string([executablePath UTF8String]);
    
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
