#!/bin/sh

# Redirect all output (stdout and stderr) to a log file
LOG_FILE="tester.log"
exec > "$LOG_FILE" 2>&1

# Get the full path of the current directory
CURRENT_DIR=$(pwd)
echo "Current directory: $CURRENT_DIR"

cd $(pwd)/roms/tools/
CURRENT_DIR=$(pwd)
echo "Current directory after cd: $CURRENT_DIR"


# Define the Tester directory path
TESTER_DIR="$CURRENT_DIR/Tester"
echo "Tester directory path: $TESTER_DIR"

# Check if Tester directory exists
if [ ! -d "$TESTER_DIR" ]; then
  echo "Error: Tester directory does not exist at $TESTER_DIR"
  exit 1
fi

# Check read and write permissions for the Tester directory
if [ ! -r "$TESTER_DIR" ] || [ ! -w "$TESTER_DIR" ]; then
  echo "Tester directory does not have read/write permissions. Updating permissions..."
  chmod -R u+rw "$TESTER_DIR"
  if [ $? -ne 0 ]; then
    echo "Failed to update permissions for $TESTER_DIR"
    exit 1
  else
    echo "Permissions successfully updated for $TESTER_DIR"
  fi
fi

# Navigate to the Tester directory
cd "$TESTER_DIR"
echo "Changed directory to $TESTER_DIR"

#/etc/init.d/S31emulationstation stop
chvt 1
# Run the Tester executable
echo "Running Tester executable..."
./TestApp "$@"
#/etc/init.d/S31emulationstation start
chvt 2
