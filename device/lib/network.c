// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

/**
@file network.c
*/

#include "defines.h"

#ifdef SE_ON_SPHERE_A7
#include <applibs/networking.h>
#include <curl/curl.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>  // strerror

#include "network.h"
#include "util_print.h"

bool is_network_connected(void)
{
    Networking_InterfaceConnectionStatus status;
    static const char networkInterface[] = "wlan0";

    int err = Networking_GetInterfaceConnectionStatus(networkInterface, &status);
    if (err)
    {
        if (errno != EAGAIN)
        {
            printf("ERROR: Networking_GetInterfaceConnectionStatus: %d (%s)\n", errno,
                   strerror(errno));
            return false;
        }
        printf("Error: Networking stack is not ready.\n");
        return false;
    }

    // -- Use a mask to get bits of the status related to internet connection
    uint32_t connected = status & Networking_InterfaceConnectionStatus_ConnectedToInternet;
    if (!connected)
    {
        printf("Error: No internet connectivity.\n");
        return false;
    }

    return true;
}

bool is_curl_error(void *ret, const char *name)
{
    if (!ret || (*(CURLcode *)(ret) != CURLE_OK))
    {
        // -- Error occured. Print debugging info
        if (name) printf("Failed: %s. ", name);
        if (ret)
        {
            CURLcode c = *((CURLcode *)ret);
            printf("Error: %s\n", curl_easy_strerror(c));
        }
        return true;
    }
    return false;
}

void send_over_network(ZZ *data, size_t num_data_bytes)
{
    CURL *curl   = NULL;
    CURLcode ret = CURLE_OK;

    // -- First, check that we are connected to the internet
    if (!is_network_connected()) return;

    ret = curl_global_init(CURL_GLOBAL_ALL);
    if (is_curl_error(&ret, "init")) return;

    curl = curl_easy_init();
    if (is_curl_error(curl, "init")) { goto cleanup; }

    // -- Specify URL to download. Important: any change
    //    in the domain name must be reflected in the
    //    AllowedConnections capability in app_manifest.json.
    ret = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (is_curl_error(&ret, "url")) { goto cleanup; }

    ret = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    if (is_curl_error(&ret, "verbose")) { goto cleanup; }

    // -- Let cURL follow any HTTP 3xx redirects. Important:
    //    any redirection to different domain names requires
    //    that domain name to be added to app_manifest.json.
    ret = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if (is_curl_error(&ret, "follow")) { goto cleanup; }

    ret = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    if (is_curl_error(&ret, "user agent")) { goto cleanup; }

    struct curl_slist *headers = NULL;
    headers                    = curl_slist_append(headers, "Content-Type: vector<uint32_t>");

    printf("Sending the following data over the network: \n");
    print_poly("data", data, num_data_bytes / sizeof(ZZ));

    ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    if (is_curl_error(&ret, "postfields")) { goto cleanup2; }

    ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, num_data_bytes);
    if (is_curl_error(&ret, "postfieldsize")) { goto cleanup2; }

    ret = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    if (is_curl_error(&ret, "headers")) { goto cleanup2; }

    ret = curl_easy_perform(curl);
    if (is_curl_error(&ret, "post")) { goto cleanup2; }

cleanup2:
    curl_slist_free_all(curl);

cleanup:
    if (curl) curl_easy_cleanup(curl);
    curl_global_cleanup();
}

#endif
