#!/bin/bash

FILE_PATH="$1"
CONTENT="$2"

# 1. Attempt to read if it exists
if [ -f "$FILE_PATH" ]; then
    # Redirect stderr to /dev/null so system errors don't corrupt the socket stream
    cat "$FILE_PATH" 2>/dev/null
    exit $? # Returns cat's success (0) or failure code to your server program
fi

# 2. Attempt to create directories and write file
# Use '&&' so it only tries to write if mkdir succeeds
if mkdir -p "$(dirname "$FILE_PATH")" 2>/dev/null && printf '%s\n' "$CONTENT" > "$FILE_PATH" 2>/dev/null; then
    printf 'File successfully created.\n'
    exit 0
else
    # If mkdir or printf failed (e.g., Permission Denied), send a clean message to the socket
    printf 'STATUS_ERROR: Failed to write file or create directory.\n'
    exit 1 # Return a failure status code to your server program
fi
