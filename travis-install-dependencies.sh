#!/bin/bash

# This script is executed by Travis CI during the before-install phase.

before_install_linux()
{
  sudo apt-get update -qq
  sudo apt-get install cmake libphysfs-dev libsdl2-dev libopenal-dev libvorbis-dev libmodplug-dev libspeex-dev
}

before_install_osx()
{
    echo "OSX"
   # Do something?
}


case "$(uname)" in
    Linux)
        before_install_linux "${PREFIX}"
        ;;
    Darwin)
        before_install_osx "${PREFIX}"
        ;;
    *)
        echo "Unknown operating system: $(uname). Only Linux and Darwin are supported."
        exit 1
        ;;
esac