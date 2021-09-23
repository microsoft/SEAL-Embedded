#!/bin/bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

BASE_DIR=$(dirname "$0")
SE_ROOT_DIR=$BASE_DIR/../../
shopt -s globstar
clang-format -i $SE_ROOT_DIR/device/**/*.h
clang-format -i $SE_ROOT_DIR/device/**/*.c
clang-format -i $SE_ROOT_DIR/adapter/**/*.h
clang-format -i $SE_ROOT_DIR/adapter/**/*.cpp
clang-format -i $SE_ROOT_DIR/example/**/*.h
clang-format -i $SE_ROOT_DIR/example/**/*.cpp
