#!/bin/sh

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

# *****************************************************************************
# -- File: gdb_launch_sphere_a7.sh
#
# -- Description:
#   This script runs the azure sphere gdb debug tool for debugging SEAL-Embedded
#   a SEAL-Embedded application on an azure sphere device and follows the
#   instructions listed under 'Debug the Sample' at the following URL:
#
#   https://docs.microsoft.com/en-us/azure-sphere/install/qs-blink-application
#
#   This script is meant to be run in conjuction with sphere_a7_launch_cl.sh
#   (from another terminal) and targets the a7 core.
#
# -- Instructions:
#   Navigate to folder with "SEAL_EMBEDDED.out" and run this script.
#   Make sure to replace 'sysroot' with the desired sysroot version to use.
#   and edit path variables according to your setup as needed.
# *****************************************************************************

sysroot=10

SCRIPT_DIR_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
AZSPHERE_SDK_PATH="/opt/azurespheresdk"
IMAGE_DIR_PATH="./build"

# -- Make sure we are connected
$AZSPHERE_SDK_PATH/DeviceConnection/azsphere_connect.sh

# -- Lines 1, 2, 3
$AZSPHERE_SDK_PATH/Sysroots/$sysroot/tools/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-musleabi/arm-poky-linux-musleabi-gdb \
--command=$SCRIPT_DIR_PATH/commands_sphere_a7.gdb \
$IMAGE_DIR_PATH/SEAL_EMBEDDED.out
