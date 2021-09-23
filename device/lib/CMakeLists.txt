# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

set(SE_LIB_SOURCE_FILES ${SE_LIB_SOURCE_FILES}
	${CMAKE_CURRENT_LIST_DIR}/ckks_common.c
	${CMAKE_CURRENT_LIST_DIR}/ckks_sym.c
	${CMAKE_CURRENT_LIST_DIR}/ckks_asym.c
	${CMAKE_CURRENT_LIST_DIR}/fft.c
	${CMAKE_CURRENT_LIST_DIR}/fileops.c
	${CMAKE_CURRENT_LIST_DIR}/modulus.c
	${CMAKE_CURRENT_LIST_DIR}/network.c
	${CMAKE_CURRENT_LIST_DIR}/parameters.c
	${CMAKE_CURRENT_LIST_DIR}/polymodmult.c
	${CMAKE_CURRENT_LIST_DIR}/rng.c
	${CMAKE_CURRENT_LIST_DIR}/sample.c
	${CMAKE_CURRENT_LIST_DIR}/timer.c
	${CMAKE_CURRENT_LIST_DIR}/uint_arith.c
	${CMAKE_CURRENT_LIST_DIR}/ntt.c
	${CMAKE_CURRENT_LIST_DIR}/intt.c
	${CMAKE_CURRENT_LIST_DIR}/seal_embedded.c
)

add_subdirectory(shake256)
set(SE_LIB_SOURCE_FILES ${SE_LIB_SOURCE_FILES} PARENT_SCOPE)
