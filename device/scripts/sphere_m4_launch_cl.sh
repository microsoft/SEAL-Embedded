#!/bin/sh

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

# *****************************************************************************
# -- File: sphere_m4_launch_cl.sh
#
# -- Description:
#   This script runs some azure sphere commands to launch a SEAL-Embedded
#   application in debug mode on an azure sphere and follows the instructions
#   listed under 'Run the Sample' and 'Debug the Sample' at the following URL:
#
#   https://docs.microsoft.com/en-us/azure-sphere/install/qs-blink-application
#
#   Note that the component ID for the SEAL Embedded image package can be found
#   in app_manifest.json or via the following command:
#
#   azsphere image-package show --image-package SEAL_EMBEDDED.imagepackage
#
# -- Instructions:
#   Nagivate to the folder where SEAL_EMBEDDED.imagepackage is created and run
#   this script.
# *****************************************************************************

# -- Application component ID, found in app_manifest.json
comp_id=5a4b43d1-a3b0-4f9f-a9c9-30a4cc52a2f2

AZSPHERE_SDK_PATH="/opt/azurespheresdk"
IMAGE_DIR_PATH="./build"

SCRIPT_DIR_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

cp $SCRIPT_DIR_PATH/app_manifest_sphere_m4.txt app_manifest.json

# -- Make sure we are connected
$AZSPHERE_SDK_PATH/DeviceConnection/azsphere_connect.sh

# -- Uncomment this to delete any applications that were previously loaded
#    on the device first
azsphere device sideload delete

# -- If the application is already running, stop it first
# azsphere device app stop --component-id $comp_id

# -- Load the applicaiton onto the device
azsphere device sideload deploy --image-package $IMAGE_DIR_PATH/SEAL_EMBEDDED.imagepackage

# -- Start running the application in debug mode
azsphere device app start --debug-mode --component-id $comp_id
#azsphere device app start --component-id $comp_id
