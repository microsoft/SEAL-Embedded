// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file network_tests.c

Adapted from Sphere example
*/

#include "defines.h"
#ifdef SE_ON_SPHERE_A7
#ifdef SE_USE_MALLOC
#include <applibs/networking.h>
#include <curl/curl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>  // strerror

#include "network.h"
#include "util_print.h"

void test_network_basic(void)
{
    CURL *curl   = NULL;
    CURLcode res = CURLE_OK;

    // First, check if we are connected to the internet
    if (!is_network_connected()) return;

    res = curl_global_init(CURL_GLOBAL_ALL);
    if (is_curl_error(&res, "init")) return;

    curl = curl_easy_init();
    if (is_curl_error(curl, "init")) { goto cleanup; }

    // Specify URL to download. Important: any change
    // in the domain name must be reflected in the
    // AllowedConnections capability in app_manifest.json.
    res = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (is_curl_error(&res, "url")) { goto cleanup; }

    res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    if (is_curl_error(&res, "verbose")) { goto cleanup; }

    // Let cURL follow any HTTP 3xx redirects.Important:
    // any redirection to different domain names requires
    // that domain name to be added to app_manifest.json.
    res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (is_curl_error(&res, "follow")) { goto cleanup; }

    res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    if (is_curl_error(&res, "user agent")) { goto cleanup; }

    //
    // First, try a GET
    //
    res = curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    if (is_curl_error(&res, "httpget")) { goto cleanup; }

    res = curl_easy_perform(curl);
    if (is_curl_error(&res, "get")) { goto cleanup; }

    //
    // Then, try a POST
    //
    const size_t num_data_bytes = 1 * sizeof(ZZ);
    ZZ *data                    = malloc(num_data_bytes);
    data[0]                     = 5;

    struct curl_slist *headers = NULL;
    headers                    = curl_slist_append(headers, "Content-Type: vector<uint64_t>");

    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    if (is_curl_error(&res, "postfields")) { goto cleanup2; }

    res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, num_data_bytes);
    if (is_curl_error(&res, "postfieldsize")) { goto cleanup2; }

    res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (is_curl_error(&res, "headers")) { goto cleanup2; }

    res = curl_easy_perform(curl);
    if (is_curl_error(&res, "post")) { goto cleanup2; }

cleanup2:
    if (data)
    {
        free(data);
        data = 0;
    }
    curl_slist_free_all(curl);

cleanup:
    if (curl) curl_easy_cleanup(curl);
    curl_global_cleanup();
}

void test_network(size_t len)
{
    const size_t num_data_bytes = len * sizeof(ZZ);
    ZZ *data                    = malloc(num_data_bytes);

    for (size_t i = 0; i < len; i++) data[i] = (ZZ)i;
    send_over_network(data, num_data_bytes);
    if (data)
    {
        free(data);
        data = 0;
    }
}
#endif
#endif
