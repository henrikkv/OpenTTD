#!/bin/bash

# Default values
ADMIN_PASSWORD="testing123"
DEBUG_LEVEL="9"
PORT="3979"
SERVER_NAME="Custom Gamemode Server"
LOAD_GAME=""

# Source environment variables from .env file
if [ -f "../.env" ]; then
    set -a  # Automatically export all variables
    source "../.env"
    set +a
else
    echo "Error: .env file not found in parent directory"
    exit 1
fi

cd ../build

cmake .. && make -j32 && ./openttd \
    -D \
    -d misc=3 \
    -X \
    -g welcome_message \
    -c ../gamemode/openttd.cfg \
    -p ../gamemode 
