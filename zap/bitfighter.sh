#!/bin/bash

launch_dir="$( dirname $0 )" # normalize the current directory
cd "$launch_dir"

datadir="../Resources"
userdatadir="$HOME/.bitfighter"

# create settings dir in users home directory
if [ ! -d "$userdatadir/robots" ]; then
  mkdir "$userdatadir"
  cp -r "$datadir/screenshots" "$userdatadir/"
  cp -r "$datadir/levels" "$userdatadir/"
  cp -r "$datadir/robots" "$userdatadir/"
  ln -s "$userdatadir" "$HOME/Documents/bitfighter_settings"
fi

# Full path is need on some Mac systems for sfx - not sure why yet
cd "$datadir"
absolute_datadir="$( pwd )"
cd -

sfxdir="$absolute_datadir/sfx"
scriptsdir="$absolute_datadir/scripts"

# one script to rule them all
exe="Bitfighter"
args="-rootDataDir $userdatadir -sfxdir $sfxdir -scriptsdir $scriptsdir $@"
if [ -f "Bitfighterd" ]; then
  dsdir="$userdatadir/dedicated_server"
  
  if [ ! -d "$dsdir" ]; then
    mkdir -p "$userdatadir/dedicated_server"
  fi
  
  exe="Bitfighterd"
  args="-rootDataDir $userdatadir -sfxdir $sfxdir -scriptsdir $scriptsdir -logdir $dsdir -inidir $dsdir $@"
fi

# Run the program
./"$exe" "$args"
