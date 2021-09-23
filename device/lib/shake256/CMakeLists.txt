# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

set(SE_LIB_SOURCE_FILES ${SE_LIB_SOURCE_FILES}
	${CMAKE_CURRENT_LIST_DIR}/fips202.c
)

if (NOT SE_BUILD_LOCAL)
    set(SE_LIB_SOURCE_FILES ${SE_LIB_SOURCE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/keccakf1600.asm
    )
else()
    set(SE_LIB_SOURCE_FILES ${SE_LIB_SOURCE_FILES}
        ${CMAKE_CURRENT_LIST_DIR}/keccakf1600.c
    )
endif()

set(SE_LIB_SOURCE_FILES ${SE_LIB_SOURCE_FILES} PARENT_SCOPE)
