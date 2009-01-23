//-----------------------------------------------------------------------------------
//
//   Torque Network Library - ZAP example multiplayer vector graphics space game
//   Copyright (C) 2004 GarageGames.com, Inc.
//   For more information see http://www.opentnl.org
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   For use in products that are not compatible with the terms of the GNU 
//   General Public License, alternative licensing options are available 
//   from GarageGames.com.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software
//   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//------------------------------------------------------------------------------------

#include <OpenGL/gl.h>

#import "MyOpenGLView.h"
#include "testGame.h"

class TestGameLog : public LogConsumer
{
   id myView;
public:
   TestGameLog(id aView) { myView = aView; }
   void logString(const char *string)
   {
      printf("%s\n", string); //[myView insertLogText: avar("%s\n", string)];
   }
};

TestGameLog *theLog;

@implementation MyOpenGLView

/*
	Override NSView's initWithFrame: to specify our pixel format:
*/	


- (id) initWithFrame: (NSRect) frame
{
	GLuint attribs[] = 
	{
		NSOpenGLPFANoRecovery,
		NSOpenGLPFAWindow,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 24,
		NSOpenGLPFAStencilSize, 8,
		NSOpenGLPFAAccumSize, 0,
		0
	};
   [NSTimer scheduledTimerWithTimeInterval: 0.02 target: self selector:@selector(tick) userInfo:NULL repeats: true];
   
	NSOpenGLPixelFormat* fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes: (NSOpenGLPixelFormatAttribute*) attribs]; 
	
	if (!fmt)
		NSLog(@"No OpenGL pixel format");

   theLog = new TestGameLog(self);
   [self restartAsClient:self];   
	return self = [super initWithFrame:frame pixelFormat: [fmt autorelease]];
}

- (void) tick
{
   if(clientGame)
      clientGame->tick();
   if(serverGame)
      serverGame->tick();
	[self setNeedsDisplay: YES];
}

- (void) awakeFromNib
{

}

/*
	Override the view's drawRect: to draw our GL content.
*/	 

- (void) drawRect: (NSRect) rect
{
   if(clientGame)
      clientGame->renderFrame((U32) rect.size.width, (U32) rect.size.height);
   else if(serverGame)
      serverGame->renderFrame((U32) rect.size.width, (U32) rect.size.height);
      
	[[self openGLContext] flushBuffer];
}

- (void)insertLogText: (const char *) text
{
   [textWindow insertText: [NSString stringWithCString:text]];
}

- (void)mouseDown:(NSEvent *)theEvent
{
   NSPoint mouseLoc;
   mouseLoc = [self convertPoint:[theEvent locationInWindow] fromView:nil];
   NSRect tempRect = [self bounds];
   Position p;
   p.x = mouseLoc.x / NSWidth(tempRect);
   p.y = 1.0f - mouseLoc.y / NSHeight(tempRect);
   if(clientGame)
      clientGame->moveMyPlayerTo(p);
   else if(serverGame)
      serverGame->moveMyPlayerTo(p);
}

- (void)restartAsServer:sender
{
   delete serverGame;
   delete clientGame;
   
   serverGame = new TestGame(true, Address(IPProtocol, Address::Any, 28999), 
                                   Address("IP:Broadcast:28999"));
   clientGame = NULL;
}

- (void)restartAsClientPingingLocalhost:sender
{
   delete serverGame;
   delete clientGame;
   serverGame = NULL;
   clientGame = new TestGame(false, Address(IPProtocol, Address::Any, 0),
                                    Address("IP:localhost:28999"));
}

- (void)restartAsClient:sender
{
   delete serverGame;
   delete clientGame;
   serverGame = NULL;
   clientGame = new TestGame(false, Address(IPProtocol, Address::Any, 0),
                                    Address("IP:broadcast:28999"));
}

- (void)restartAsClientAndServer:sender
{
   delete serverGame;
   delete clientGame;
   serverGame = new TestGame(true, Address(IPProtocol, Address::Any, 28999), 
                                   Address("IP:Broadcast:28999"));
   clientGame = new TestGame(false, Address(IPProtocol, Address::Any, 0),
                                    Address("IP:broadcast:28999"));
                                    
   clientGame->createLocalConnection(serverGame);
}

@end