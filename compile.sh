#!/bin/sh
cd build
cmake ..
make VERBOSE=1
cd ..
mv build/Weed-Spotter Weed-Spotter

