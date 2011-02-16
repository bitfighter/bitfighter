#!/bin/bash

launch_dir="$( dirname $0 )" # normalize the current directory
cd "$launch_dir"

datadir="../Resources"
userdatadir="$HOME/Library/Application Support/Bitfighter"

# Am I dedicated?  For Zoomber...
dedicated=0
if [ -f "Bitfighterd" ]; then
  dedicated=1
fi

# create settings dir in users home directory
if [ ! -d "$userdatadir/robots" ]; then
  mkdir "$userdatadir"
  mkdir "$userdatadir/screenshots"
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

dsdir=""
if [ $dedicated -eq 1 ]; then
  dsdir="$userdatadir/dedicated_server"
  
  if [ ! -d "$dsdir" ]; then
    mkdir -p "$dsdir"
  fi 
fi

# Run the program
if [ $dedicated -eq 1 ]; then
  ./Bitfighterd -rootDataDir "$userdatadir" -sfxdir "$sfxdir" -scriptsdir "$scriptsdir" -logdir "$dsdir" -inidir "$dsdir" "$@"
else
  ./Bitfighter -rootDataDir "$userdatadir" -sfxdir "$sfxdir" -scriptsdir "$scriptsdir" "$@"
fi