#!/bin/bash

# This script is executed by Travis CI during the before-install phase.

set -o errexit # exit on first error
set -o nounset # report unset variables
set -o xtrace  # show commands

before_install_linux()
{
  sudo apt-get update -qq
  sudo apt-get install cmake libphysfs-dev libsdl2-dev libopenal-dev libvorbis-dev libmodplug-dev libspeex-dev
}

before_install_osx()
{
   # Do something?
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 <installation prefix>"
    exit 1
fi

export PREFIX="$1"
mkdir -p "${PREFIX}"


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