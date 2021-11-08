#!/bin/sh

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

# *****************************************************************************
# -- File: gdb_launch_sphere_m4.sh
#
# -- Description:
#   This script runs the azure sphere gdb debug tool for debugging SEAL-Embedded
#   a SEAL-Embedded application on an azure sphere device and follows the
#   instructions listen under 'Debug the Sample' at the following URL:
#
#   https://docs.microsoft.com/en-us/azure-sphere/install/qs-real-time-application

#   This script is meant to be run in conjuction with sphere_m4_launch_cl.sh
#   (from another terminal) and targets the m4 core.
#
# -- Instructions:
#   Navigate to folder with "SEAL_EMBEDDED.out" and run this script.
#   Make sure to replace 'sysroot' with the desired sysroot version to use.
#   and edit path variables according to your setup as needed.
# *****************************************************************************

sysroot=10

SCRIPT_DIR_PATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
GNU_TOOLCHAIN_PATH="${HOME}/gcc-arm-none-eabi-10.3-2021.07"
IMAGE_DIR_PATH="./build"

$GNU_TOOLCHAIN_PATH/bin/arm-none-eabi-gdb \
--command=$SCRIPT_DIR_PATH/commands_sphere_m4.gdb \
$IMAGE_DIR_PATH/SEAL_EMBEDDED.out
