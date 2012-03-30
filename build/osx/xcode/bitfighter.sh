#!/bin/bash
#
# This is the Mac OS X wrapper script.  It is ugly.  It is definitely
# the *wrong* way to do things on Mac - but it's good enough for now

launch_dir="$( dirname "$0" )" # normalize the current directory
cd "$launch_dir"

datadir="../Resources"
userdatadir="$HOME/Library/Application Support/Bitfighter"

## First time run
# create settings dir in users home directory and copy resources
if [ ! -d "$userdatadir/robots" ]; then
  mkdir "$userdatadir"
  mkdir "$userdatadir/screenshots"
  mkdir "$userdatadir/music"
  cp -r "$datadir/levels" "$userdatadir/"
  cp -r "$datadir/robots" "$userdatadir/"
  cp -r "$datadir/scripts" "$userdatadir/"
  cp -r "$datadir/editor_plugins" "$userdatadir/"
  cp -r "$datadir/music" "$userdatadir/"
  ln -s "$userdatadir" "$HOME/Documents/bitfighter_settings"
fi

## Upgrade specifics
# 015a
if [ ! -f "$userdatadir/robots/s_bot.bot" ]; then
  cp "$datadir/robots/s_bot.bot" "$userdatadir/robots/"
fi

# 016
if [ ! -d "$userdatadir/scripts" ]; then
  cp -r "$datadir/scripts" "$userdatadir/"
fi
if [ ! -d "$userdatadir/editor_plugins" ]; then
  cp -r "$datadir/editor_plugins" "$userdatadir/"
fi
if [ ! -f "$userdatadir/joystick_presets.ini" ]; then
  cp "$datadir/joystick_presets.ini" "$userdatadir/"
fi
if [ ! -f "$userdatadir/levels/core.level" ]; then
  cp -f "$datadir/levels/"* "$userdatadir/levels/"
fi

# 017
if [ ! -f "$userdatadir/music/menu.ogg" -o ! -f "$userdatadir/music/game.ogg" ]; then
  cp -r "$datadir/music" "$userdatadir/"
  # Just once upgrade s_bot
  cp -f "$datadir/robots/s_bot.bot" "$userdatadir/robots/"
fi


# Full path is need on some Mac systems for sfx - not sure why yet
cd "$datadir"
absolute_datadir="$( pwd )"
cd -

sfxdir="$absolute_datadir/sfx"

# Run the program
./Bitfighter -rootdatadir "$userdatadir" -sfxdir "$sfxdir" 
