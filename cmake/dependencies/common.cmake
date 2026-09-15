# =================== SSE2NEON ===================
set(SSE2NEON_DIR ${CMAKE_BINARY_DIR}/_deps/sse2neon)
set(SSE2NEON_PATH "${SSE2NEON_DIR}/sse2neon.h")
set(SSE2NEON_SHA256
    fab5e1be095ce8db9d2ab0c1fe0b2c6f2a34657b43ce83688e8c84224806df38)
if(EXISTS "${SSE2NEON_PATH}")
  file(SHA256 "${SSE2NEON_PATH}" SSE2NEON_EXISTING_SHA256)
endif()
if(NOT SSE2NEON_EXISTING_SHA256 STREQUAL SSE2NEON_SHA256)
  file(
    DOWNLOAD
    "https://raw.githubusercontent.com/DLTcollab/sse2neon/3b70b3727edc9a151c113814129258c3423a771c/sse2neon.h"
    "${SSE2NEON_PATH}"
    EXPECTED_HASH SHA256=${SSE2NEON_SHA256}
    TLS_VERIFY ON)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${SSE2NEON_DIR})

# ================== SEMVER ===================
set(SEMVER_DIR ${CMAKE_BINARY_DIR}/_deps/semver)
set(SEMVER_PATH "${SEMVER_DIR}/semver.hpp")
set(SEMVER_SHA256
    af2c0c53124dc7f52c58a7205e458ad3efbac2f61ce55addf9c8f94338a04182)
if(EXISTS "${SEMVER_PATH}")
  file(SHA256 "${SEMVER_PATH}" SEMVER_EXISTING_SHA256)
endif()
if(NOT SEMVER_EXISTING_SHA256 STREQUAL SEMVER_SHA256)
  file(
    DOWNLOAD
    "https://raw.githubusercontent.com/Neargye/semver/ccbbfdf2f862a48498d37d5955743a5082ee49bf/include/semver.hpp"
    "${SEMVER_PATH}"
    EXPECTED_HASH SHA256=${SEMVER_SHA256}
    TLS_VERIFY ON)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${SEMVER_DIR})

# =================== DRLibs ===================
FetchContent_Declare(
  dr_libs
  GIT_REPOSITORY https://github.com/mackron/dr_libs.git
  GIT_TAG da35f9d6c7374a95353fd1df1d394d44ab66cf01)
FetchContent_MakeAvailable(dr_libs)

target_include_directories(${PROJECT_NAME} PRIVATE ${dr_libs_SOURCE_DIR})

# =================== tomlplusplus ===================
include(FetchContent)
FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0)
FetchContent_MakeAvailable(tomlplusplus)
target_link_libraries(${PROJECT_NAME} PRIVATE tomlplusplus::tomlplusplus)

# libultraship
# Removes MPQ/OTR support
set(EXCLUDE_MPQ_SUPPORT TRUE CACHE BOOL "")
set(ENABLE_EXP_AUTO_CONFIGURE_CONTROLLERS ON CACHE BOOL "")
target_compile_definitions(${PROJECT_NAME} PRIVATE EXCLUDE_MPQ_SUPPORT)

target_include_directories(
  ${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include
  ${CMAKE_CURRENT_SOURCE_DIR}/libultraship/include/libultraship)

add_subdirectory(libultraship ${CMAKE_BINARY_DIR}/libultraship)
add_dependencies(${PROJECT_NAME} libultraship)
target_link_libraries(${PROJECT_NAME} PRIVATE libultraship)

# Torch
option(USE_STANDALONE "Build as a standalone executable" OFF)
option(BUILD_STORMLIB "Build with StormLib support" OFF)

option(BUILD_SM64 "Build with Super Mario 64 support" OFF)
option(BUILD_MK64 "Build with Mario Kart 64 support" ON)
option(BUILD_SF64 "Build with Star Fox 64 support" OFF)
option(BUILD_FZERO "Build with F-Zero X support" OFF)
option(BUILD_MARIO_ARTIST "Build with Mario Artist support" OFF)
# Companion.cpp uses AudioManager unconditionally, so preserve Torch's ON
# default until its non-NAudio build is fixed upstream.
option(BUILD_NAUDIO "Build with NAudio support" ON)

add_subdirectory(torch)
