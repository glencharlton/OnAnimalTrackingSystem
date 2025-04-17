#!/bin/bash

# Move to the file location


# Activate the virtual environment
source dbSync/venv/bin/activate

# Run the Python script and store the result
result=$(python dbSync/dbConnected.py)

# Deactivate the virtual environment
deactivate

# Return the result from the Python script
echo "$result"
