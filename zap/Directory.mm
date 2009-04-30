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
#include "tnl/tnlVector.h"

using TNL::Vector;
using std::string;


bool getLevels(string subdir, Vector<string> &files)
{
    //subdir = "levels/" + subdir;
    
    //Since this isn't an actual Cocoa app, we need an AutoreleasePool for the
    //durations of these functions.
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSFileManager *fm = [NSFileManager defaultManager];
    NSArray *filelist = [fm directoryContentsAtPath:[NSString stringWithCString:subdir.c_str()]];
    
    if (filelist == nil)
    {
        [pool release];
        
        return false;
    } 
    
    NSEnumerator *i = [filelist objectEnumerator];
    
    while (id object = [i nextObject])
    {
        if ([object length] > 6 && [object hasSuffix:@".level"])
        {
            files.push_back([object UTF8String]);
        }
    }
    
    [pool release];
    
    return !files.empty();
}

void moveToAppPath()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSFileManager *fm = [NSFileManager defaultManager];
    
	//On load, change to the application directory so we can get to the graphics/sounds/etc...
	[fm changeCurrentDirectoryPath:[[[NSBundle mainBundle] bundlePath] stringByDeletingLastPathComponent]];
    
    [pool release];
}
