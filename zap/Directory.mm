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
#import <Cocoa/Cocoa.h>
#include "tnlVector.h"
#import "SUUpdater.h"
#define SPARKLE_APPCAST_URL @"http://127.0.0.1" //TODO: specify right url to appcast

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

void prepareFirstLaunch()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSFileManager *fm = [NSFileManager defaultManager];

    NSArray *appSupportPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
    NSString *bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    NSString *fullAppSupportPath = [NSString stringWithFormat:@"%@/%@", [appSupportPaths objectAtIndex:0], bundleName];

    //First check if ~/Library/Application Support/Bitfighter exists, if so do nothing
    BOOL isDirectory = NO;
    if ([fm fileExistsAtPath:fullAppSupportPath isDirectory:&isDirectory] && isDirectory)
        return;

    //Then, create basic directories
    NSString *screenshotsPath = [fullAppSupportPath stringByAppendingPathComponent:@"screenshots"];
    if ([fm respondsToSelector:@selector(createDirectoryAtPath:withIntermediateDirectories:attributes:error:)])
    {
        [fm createDirectoryAtPath:fullAppSupportPath
      withIntermediateDirectories:YES
                       attributes:nil
                            error:NULL];
        [fm createDirectoryAtPath:screenshotsPath
      withIntermediateDirectories:YES
                       attributes:nil
                            error:NULL];
    }
    else
    {
        [fm createDirectoryAtPath:fullAppSupportPath attributes:nil];
        [fm createDirectoryAtPath:screenshotsPath attributes:nil];
    }

    //And finally copy resources
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    NSArray *pathsToCreate = [NSArray arrayWithObjects:@"levels",@"robots",@"scripts",@"editor_plugins",@"music",nil];
    for (int i = 0; i < [pathsToCreate count]; i++)
    {
        NSString *path = [pathsToCreate objectAtIndex:i];
        if ([fm respondsToSelector:@selector(copyItemAtPath:toPath:error:)])
            [fm copyItemAtPath:[resourcePath stringByAppendingPathComponent:path]
                        toPath:[fullAppSupportPath stringByAppendingPathComponent:path]
                         error:NULL];
        else
            [fm copyPath:[resourcePath stringByAppendingPathComponent:path]
                  toPath:[fullAppSupportPath stringByAppendingPathComponent:path]
                 handler:nil];
    }

    //NOTE: intentionally not backporting `ln -s "$userdatadir" "$HOME/Documents/bitfighter_settings"`
    //TODO: "Upgrade specifics" sections need to be backported in some way
    [pool release];
}

void checkForUpdates()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    SUUpdater* updater = [SUUpdater sharedUpdater];
    [updater setFeedURL:[NSURL URLWithString:SPARKLE_APPCAST_URL]];
    [updater checkForUpdatesInBackground];
    [pool release];
}

void setDefaultPaths(Vector<string> &argv)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    if (argv.contains("-rootdatadir") == NO) {
        argv.push_back("-rootdatadir");
        NSArray *appSupportPaths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
        NSString *bundleName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
        argv.push_back([[NSString stringWithFormat:@"%@/%@", [appSupportPaths objectAtIndex:0], bundleName] UTF8String]);
    }
    if (argv.contains("-sfxdir") == NO) {
        argv.push_back("-sfxdir");
        argv.push_back([[NSString stringWithFormat:@"%@/sfx",[[NSBundle mainBundle] resourcePath]] UTF8String]);
    }
    [pool release];
}
