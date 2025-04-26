#!/bin/bash

# Move to the file location


# Activate the virtual environment
source location_data_processing/venv/bin/activate

# Run the Python script and store the result
result=$(python location_data_processing/gnss_processing.py)

# Deactivate the virtual environment
deactivate

# Return the result from the Python script
echo "$result"
