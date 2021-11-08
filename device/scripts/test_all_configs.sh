#!/bin/bash

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

CHANGE_DEFINES_SCRIPT=./scripts/change_defines.py
BUILD_DIR=./build

# ifft            0, 1
# ntt             0, 1, 2, 3
# index_map_type  0, 1, 2, 3, 4
# sk_type         0, 1, 2
# data_load_type  0, 1

# python3 $CHANGE_DEFINES_SCRIPT -i 0 -n 0 -m 0 -s 0 -d 0
# cmake --build build -j || exit 1
# $BUILD_DIR/bin/seal_embedded_tests
# status=$?
# echo "status = "$status
# if [ $status -ne "0" ] 
# then 
#     echo "THERE WAS AN ERROR."
#     exit
# fi
# exit

for d in 0 1
    do
    for i in 0 1
        do
            for n in 0 1 2 3
                do
                    for m in 0 1 2 3 4
                        do
                            for s in 0 1
                                do
                                    python3 $CHANGE_DEFINES_SCRIPT -i $i -n $n -m $m -s $s -d $d 
                                    cmake --build $BUILD_DIR -j || exit 1
                                    $BUILD_DIR/bin/seal_embedded_tests
                                    status=$?
                                    echo "status = "$status
                                    if [ $status -ne 0 ] 
                                    then 
                                        echo "THERE WAS AN ERROR."
                                        exit
                                    fi
                                done
                        done
                done
        done
    done
