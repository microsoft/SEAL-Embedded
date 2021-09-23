# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

set(SE_TESTS_SOURCE_FILES ${SE_TESTS_SOURCE_FILES}
	${CMAKE_CURRENT_LIST_DIR}/fft_tests.c
	${CMAKE_CURRENT_LIST_DIR}/modulo_tests.c
	${CMAKE_CURRENT_LIST_DIR}/network_tests.c
	${CMAKE_CURRENT_LIST_DIR}/sample_tests.c
	${CMAKE_CURRENT_LIST_DIR}/uintmodarith_tests.c
	${CMAKE_CURRENT_LIST_DIR}/uintops_tests.c
	${CMAKE_CURRENT_LIST_DIR}/ntt_tests.c
	${CMAKE_CURRENT_LIST_DIR}/ckks_tests_encode.c
	${CMAKE_CURRENT_LIST_DIR}/ckks_tests_common.c
	${CMAKE_CURRENT_LIST_DIR}/ckks_tests_asym.c
	${CMAKE_CURRENT_LIST_DIR}/ckks_tests_sym.c
	${CMAKE_CURRENT_LIST_DIR}/api_tests.c
	${CMAKE_CURRENT_LIST_DIR}/main.c
)

set(SE_TESTS_SOURCE_FILES ${SE_TESTS_SOURCE_FILES} PARENT_SCOPE)
