#!/bin/bash

# This script is executed by Travis CI during the before-install phase.

before_install_linux()
{
  sudo apt-get update -qq
  sudo apt-get install cmake libphysfs-dev libsdl2-dev libopenal-dev libvorbis-dev libmodplug-dev libspeex-dev
}

before_install_osx()
{
  # Remove boost because we're using old, in-tree version for now
  brew uninstall boost --force
}


case "${TRAVIS_OS_NAME}" in
    linux)
        before_install_linux "${PREFIX}"
        ;;
    osx)
        before_install_osx "${PREFIX}"
        ;;
    *)
        echo "Unknown operating system: ${TRAVIS_OS_NAME}. Only Linux and OSX are supported."
        exit 1
        ;;
esac
