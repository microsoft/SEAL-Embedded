# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT license.

set(SE_BENCH_SOURCE_FILES ${SE_BENCH_SOURCE_FILES}
	${CMAKE_CURRENT_LIST_DIR}/bench_sym.c
	${CMAKE_CURRENT_LIST_DIR}/bench_asym.c
	${CMAKE_CURRENT_LIST_DIR}/bench_ntt.c
	${CMAKE_CURRENT_LIST_DIR}/bench_ifft.c
	${CMAKE_CURRENT_LIST_DIR}/bench_sample.c
	${CMAKE_CURRENT_LIST_DIR}/bench_index_map.c
	${CMAKE_CURRENT_LIST_DIR}/main.c
)

set(SE_BENCH_SOURCE_FILES ${SE_BENCH_SOURCE_FILES} PARENT_SCOPE)
