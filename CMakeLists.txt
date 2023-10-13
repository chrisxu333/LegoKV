include(GoogleTest)

cmake_minimum_required(VERSION 3.14)
project(legokv
    DESCRIPTION "Component-based LSM storage engine"
    LANGUAGES CXX C ASM)

# RepDB version number
set (REPDB_VERSION_MAJOR 1)
set (REPDB_VERSION_MINOR 0)

# CXX related
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wextra -Wall -pthread")

# include path
include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/components)
set(COMPONENT_BASE src/components)
set(SRC_BASE src)

# misc names
set(LEGOKV_STATIC_LIB legokv${ARTIFACT_SUFFIX})
set(LEGOKV_SHARED_LIB legokv-shared${ARTIFACT_SUFFIX})

# sources
set(SOURCES
    ${COMPONENT_BASE}/util/coding/coding.cc
    ${COMPONENT_BASE}/util/hashing/hash.cc
    ${COMPONENT_BASE}/util/hashing/xxhash.cc
    ${COMPONENT_BASE}/util/random/random.cc
    ${COMPONENT_BASE}/util/stringutil/string_util.cc
    # ${COMPONENT_BASE}/util/bloom_filter/dynamic_bloom.cc
    # ${COMPONENT_BASE}/memory/arena.cc
    # ${COMPONENT_BASE}/memory/concurrent_arena.cc
    ${SRC_BASE}/port/port_posix.cc
    ${SRC_BASE}/port/mmap.cc
    ${SRC_BASE}/port/stack_trace.cc
    ${SRC_BASE}/status.cc
    ${SRC_BASE}/slice.cc

    # ${SRC_BASE}/test_util/sync_point.cc
    # ${SRC_BASE}/test_util/sync_point_impl.cc
    # ${SRC_BASE}/test_util/testharness.cc

    ${SRC_BASE}/hotring_store/hotring_index.cc
)

# build static library
if(LEGOKV_BUILD_STATIC)
    add_library(${LEGOKV_STATIC_LIB} STATIC ${SOURCES})
    set(LEGOKV_LIB ${LEGOKV_STATIC_LIB})
else()
    add_library(${LEGOKV_SHARED_LIB} SHARED ${SOURCES})
    set(LEGOKV_LIB ${LEGOKV_SHARED_LIB})
endif()

# enable testing
if(WITH_TESTS)
  include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/third-party/)
endif(WITH_TESTS)

if(WITH_TESTS)
    add_subdirectory(third-party/gtest)
    enable_testing()

    set(TEST_SOURCE
        ${COMPONENT_BASE}/util/random/random_test.cc
        ${COMPONENT_BASE}/util/hashing/hash_test.cc
        # ${COMPONENT_BASE}/util/coding/coding_test.cc
        # ${COMPONENT_BASE}/util/autovector/autovector_test.cc
        # ${COMPONENT_BASE}/memory/arena_test.cc
        ${SRC_BASE}/hotring_store/hotring_index_test.cc
    )
    
    foreach(src ${TEST_SOURCE})
        get_filename_component(exename ${src} NAME_WE)
        add_executable(${exename} ${src})
        target_link_libraries(${exename} ${LEGOKV_LIB} gtest)
        gtest_discover_tests(${exename} DISCOVERY_TIMEOUT 120)
    endforeach(src ${TEST_SOURCE})
endif(WITH_TESTS)