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
AZSPHERE_SDK_PATH="/opt/azurespheresdk"
core="targets io0"

# -- Replace  above line with this if targetting Core 1 instead
# core="targets io1"

# -- Save current path
# dir=$(pwd)

# -- Change directories
cd $AZSPHERE_SDK_PATH/Sysroots/$sysroot/tools/sysroots/x86_64-pokysdk-linux/

# -- Run the on-chip debugger
./usr/bin/openocd -f mt3620-rdb-ftdi.cfg -f mt3620-io0.cfg -c "gdb_memory_map disable" -c "gdb_breakpoint_override hard" -c init -c "${core}" -c halt -c "targets"
