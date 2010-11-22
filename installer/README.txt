Bitfighter Readme

Introduction
------------

Welcome to Bitfighter, the retro multiplayer team action game!  Bitfighter is a
game of action and strategy.  In Bitfighter, the goal of the game varies
from level to level, from the following game types:

Capture the Flag - Team game where the objective is to take the
enemy's flag and return it to your flag.  Each capture earns your
team one point.  Take care to defend your flag from the enemy -- 
you can only score if it is at home!

Soccer - Team game where the objective is to move the white circle
(the ball) into the goal of the opponent's color.

Zone Control - Team game with a single flag and multiple "capture zone"
areas.  The goal of the game is to escort a team member carrying the
flag to each of the capture zones in the map.  When a capture zone
is entered by the flag carrier, the zone turns to that team's color
and the team scores a point.  If the capture zone was already owned
by a different team, the team that lost the zone loses a point.  If
all the zones on the level are captured by a single team, that scores
a "touchdown", and the zones and flag reset.

Retrieve - In Retrieve, one or more flags are scattered throughout the level.
Teams compete to bring these flags back to team-colored goal zones.  Each
retrieved flag is worth one point to the capturing team.  If a team retrieves
all the flags on the level, the team keeps the points for the flags and the
flags reset to their original locations.

Nexus - Solo game where the objective is to collect flags from
other players and return them to the Nexus for points.  Each player
starts with one flag, and drops it if he or she is zapped.  Scoring
in Hunters is based on how many flags the player is carrying when
touching the open Nexus.  The first flag is worth one point, the
second is worth two, the third 3, and so on.  So the total value
of capturing 5 flags would be 5 + 4 + 3 + 2 + 1 = 15 points.  If
the Nexus is dark, it is closed -- the upper timer in the lower
right corner counts down to when it will next be open.

Rabbit - Solo game wherein there is one flag that all players are fighting
to control.  Players accumulate points by holding the flag, zapping
the flag carrier or zapping other players while holding the flag.

Bitmatch - Solo game, often of short duration between levels.  Just
zap as many other players as you can!

Pay special attention to the triangular arrows that move in an ellipse around
the screen -- these indicate the direction to specific game objectives, including
flags to retrieve, zones to capture and friendly or enemy players carrying flags.

Ship configuration:
Each ship can be configured with 2 modules and 3 weapons.  Pressing
the loadout select screen allows the player to choose the next loadout
for his or her ship.  This loadout will not become available until the
player either flies over a resupply area (team-color-coded patch), or
respawns (only if there are no resupply areas on the level).

Modules are special powers that can be activated by pressing the appropriate
module activation key.  The modules in Bitfighter, and their function are:

1. Boost - Gives the ship a boost of speed
2. Shield - Creates a defensive barrier around the ship that reflects shots
3. Repair - Repairs self and nearby teammates that are damaged
4. Sensor - Boosts the screen visible distance of the player
5. Cloak - Turns the ship invisible

Controls:

w - move up
S - move down
A - move left
D - move right

T - chat to team
G - chat global
V - open quick chat menu
R - record voice chat

C - toggle commander map
SPACE - activate primary module (default = boost)
SHIFT - activate secondary module (default = shield)

TAB - show scores

E - next weapon
Z - select weapon and module loadout

Mouse:

Mouse button 1 - fire weapon
Mouse button 2 - activate secondary module

Dual-Analog Controller:

Left stick - move
Right stick - aim and fire

Buttons (Logitech Wingman Cordless)
A - cycle to next weapon
B - toggle commander map
C - open quick chat menu
X - select weapon and module loadout 
Y - show scores
Z - record voice
right trigger - activate primary module (default = boost)
left trigger - activate secondary module (default = shield)

Logitech Dual Action
1 - cycle to next weapon
2 - toggle commander map
3 - open quick chat menu
4 - select weapon and module loadout 
5 - activate primary module (default = boost)
6 - activate secondary module (default = shield)
7 - show scores
8 - record voice

Saitek P880 Dual Analog
1 - cycle to next weapon
2 - toggle commander map
3 - open quick chat menu
4 - select weapon and module loadout 
5 - show scores
6 - record voice
right trigger - activate primary module (default = boost)
left trigger - activate secondary module (default = shield)

PS 2 Dual Shock w/USB
X or depress right stick - cycle to next weapon
O - toggle commander map
square - open quick chat menu
triangle - select weapon and module loadout
R1 - activate primary module (default = boost)
L1 - activate secondary module (default = shield)
L2 - show scores
R2 - record voice

XBox controller
A (green) - cycle to weapon
B (red) - toggle commander map
X (blue) - open quick chat menu
Y (yellow) - select weapon and module loadout
white - show scores
black - record voice
right trigger - activate primary module (default = boost)
left trigger - activate secondary module (default = shield)

In the game you can press ESC to go to the game options screen.  
From there you can access the main options which include setting 
full screen mode, enabling relative controls and on the Windows 
platform enabling dual analog controller support.

Command Line Options:

Note - Bitfighter addresses are of the form transport:address:port like:
IP:127.0.0.1:28000
or IP:Any:28000
or IP:www.foobar.com:24601

-server [bindAddress] hosts a game server/client on the specified 
        bind address.
-master [masterAddress] specfies the address of the master server 
        to connect to.
-dedicated [bindAddress] starts Zap as a dedicated server
-name [playerName] sets the client's name to the specified name 
        and skips the name entry screen.
-levels ["level1 level2 level3 ... leveln"] sets the specified level 
		rotation for games
-hostname [hostname] sets the name that will appear in the server 
        browser when searching for servers.
-maxplayers [number] sets the maximum number of players allowed 
        on the server
-password [password] sets the password for access to the server.
-adminpassword [password] sets the administrator password for the server.
-levelchangepassword [password] sets the password that allows players to change levels on server.
-joystick [joystickType] enables dual analog control pad.  The
        joystickType argument can be either 0, 1 or 2.  If the right
        stick doesn't aim shots properly with 0, try 1 or 2.
        
        Known controllers:
		Logitech Wingman cordless - joystick 0
		Logitech Dual Action - joystick 1
		Saitek P880 Dual Analog - joystick 2
		PS 2 Dual Shock w/USB - joystick 3
		XBox controller - joystick 4
        
-jsave [journalName] saves the log of the play session to the specified
        journal file.
-jplay [journalName] replays a saved journal.
-edit [levelName] starts Bitfighter in level editing mode, loading and saving the
		specified level.
		
Level editor instructions:

Currently the level editor allows you to edit the barrier and level objects
within levels.

Mouse functions:

Left-click - select and move.  Clicking on vertices allows movement
		of verts, clicking on edges allows movement of the entire barrier border.
		To move a vertex or border, click and hold as you drag the object around.
		Left click also completes a new barrier border.
		If no object is under the mouse, left-clicking will create a 
		drag selection box for selecting multiple objects.  Holding down
		the shift key also allows multiple selection of objects.

Right-click - add barrier vertex.  If right clicking on an existing barrier edge,
		this will insert a new vertex along that edge at the click point.  Otherwise
		this either begins a new barrier border or adds a new vertex to the current
		new barrier border.

Keyboard functions:

W - scroll map up
S - scroll map down
A - scroll map left
D - scroll map right
C - zoom out
E - zoom in
R - reset view to 0,0 in the top left corner
F - flip current selection horizontally
V - flip current selection vertically

0...9 - set the active team for item construction.  This will also set the team 
	of any selected objects
T - construct a Teleporter at the mouse point
G - construct a Spawn point at the mouse point
B - construct a RepairItem at the mouse point
Y - construct a Turret at the mouse point
H - construct a Force Field projector at the mouse point
M - construct a Mine at mouse point

CTRL-D - duplicate current selection
CTRL-Z - undo last operation

ESC - bring up editor menu

Bitfighter is free software based off of the ZAP source code. 
