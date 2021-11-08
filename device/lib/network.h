// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file network.h
*/

#pragma once

#include "defines.h"

#ifdef SE_ON_SPHERE_A7
#include <stdbool.h>

// static const char *url = "http://neverssl.com";
static const char *url = "http://httpstat.us";

/**
Checks that the interface is connected to the internet.

@returns True if connected, false otherwise
*/
bool is_network_connected(void);

/**
Checks if there was an error with curl. Prints a message upon error detection (does not exit upon
error detection).

@param[in] ret   Curl-function returned value to check
@param[in] name  A name or message to print upon error detection
@returns         True if there was an error detected, false otherwise
*/
bool is_curl_error(void *ret, const char *name);

/**
Sends 'num_data_bytes' of bytes from location pointed to by 'data' over the network connection using
cURL. Prints a message upon error detection (does not exit upon error detection).

@param[in] data            Pointer to location of data to send over the network
@param[in] num_data_bytes  Number of bytes of 'data' to send
*/
void send_over_network(ZZ *data, size_t num_data_bytes);
#endif
