/*
Origin: Microsoft SEAL library (https://github.com/microsoft/SEAL) commit
2d2a25c916047d2f356d45ad50c98f0a9695905a.
*/

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <stddef.h>
#include <stdint.h>

void shake256(uint8_t *out, size_t outlen, const uint8_t *in, size_t inlen);
