#!/usr/bin/env bash

# Require superuser
if [ $(id -u) != "0" ]; then
    echo "You must be the superuser to run this script" >&2
    exit 1
fi

# Install gcc-multilib to be able to compile 32-bit code on 64-bit hosts (and vice versa)
echo "Installing gcc-multilib and g++-multilib..."
apt-get install gcc-4.8-multilib g++-4.8-multilib
echo "DONE (Installing gcc-multilib and g++-multilib)"
echo ""

# Check if the /usr/include/asm directory exists and link it to /usr/include/asm-generic if missing
if ! [ -e /usr/include/asm ]; then
    echo "Creating /usr/include/asm link to /usr/include/asm-generic..."
    ln -s /usr/include/asm-generic /usr/include/asm
    echo "DONE (Creating link)"
    echo ""
fi

# Install arm gcc
echo "Installing gcc-arm-linux-gnueabi..."
apt-get install gcc-arm-linux-gnueabi
echo "DONE (Installing gcc-arm-linux-gnueabi)"
echo ""

echo "Installing g++-arm-linux-gnueabi..."
apt-get install g++-arm-linux-gnueabi
echo "DONE (Installing g++-arm-linux-gnueabi)"
echo ""

# Install armhf gcc
echo "Installing gcc-arm-linux-gnueabihf..."
apt-get install gcc-arm-linux-gnueabihf
echo "DONE (Installing gcc-arm-linux-gnueabihf)"
echo ""

echo "Installing g++-arm-linux-gnueabihf..."
apt-get install g++-arm-linux-gnueabihf
echo "DONE (Installing g++-arm-linux-gnueabihf)"
echo ""
