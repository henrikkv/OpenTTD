#!/bin/bash

# Default values
ADMIN_PASSWORD="testing123"
DEBUG_LEVEL="9"
PORT="3979"
SERVER_NAME="Custom Gamemode Server"
LOAD_GAME=""


cd ..

cmake --build build

cd build && ./openttd \
    -D \
    -d script=3 \
    -X \
    -g welcome_message \
    -c ../gamemode/openttd.cfg \
    -p ../gamemode 
