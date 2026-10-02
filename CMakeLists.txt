cmake_minimum_required(VERSION 3.16.0 FATAL_ERROR)

# Set the project version and language
project(
  Spaghettify
  VERSION 1.0.0
  LANGUAGES C CXX ASM)
include(FetchContent)

# Set Project Version
include(cmake/GetVersion.cmake)

# Set some Cmake variable
include(cmake/SetCmakeVar.cmake)

# add_compile_options(-fsanitize=address) add_link_options(-fsanitize=address)

set(YAML_CPP_STATIC_DEFINE ON)
include(libultraship/cmake/cvars.cmake)

if(WIN32)
  option(ENABLE_VCPKG "Enable vcpkg for dependencies" ON)
else()
  option(ENABLE_VCPKG "Enable vcpkg for dependencies" OFF)
endif()

if(ENABLE_VCPKG)
  include(cmake/dependencies/vcpkg.cmake)
endif()

set(SPAGHETTIKART_IOS OFF)
if(IOS OR CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(SPAGHETTIKART_IOS ON)
  add_compile_options("-ffile-prefix-map=${CMAKE_SOURCE_DIR}=.")
  add_compile_options("-ffile-prefix-map=${CMAKE_BINARY_DIR}=build-ios")
endif()

# Toggle for GLES (Required for many ARM/Mobile platforms)
option(USE_OPENGLES "Use OpenGLES instead of Desktop OpenGL" OFF)

# Set game compilation version
set(VERSION us)
set(SKIP_XCODE_VERSION_CHECK ON)

# TODO: Sorry i broke this
set(GFX_DEBUG_DISASSEMBLER OFF)

if(MSVC)
  set(CPP "${CMAKE_C_COMPILER}" "/EP")
else()
  set(CPP "${CMAKE_C_COMPILER}" "-E" "-P" "-Wno-trigraphs" "-x" "c")
endif()

if(WIN32)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/properties.h.in
                 ${CMAKE_CURRENT_SOURCE_DIR}/properties.h @ONLY)
endif()

# Collect source files to build the executable
file(
  GLOB ALL_FILES_ROOT
  CONFIGURE_DEPENDS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "src/*.c"
  "src/*.h"
  "src/os/guLookAtF.c"
  "src/os/guPerspectiveF.c"
  "src/os/guMtxF2L.c"
  "src/os/guRotateF.c"
  "src/os/guOrthoF.c"
  "src/os/guScaleF.c"
  "src/os/guTranslateF.c"
  "src/os/guMtxCatF.c"
  "src/os/guMtxCatL.c"
  "src/os/guNormalize.c")

file(
  GLOB_RECURSE ALL_FILES
  CONFIGURE_DEPENDS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "src/buffers/*.c"
  "src/buffers/*.h"
  "src/data/*.c"
  "src/data/*.h"
  "src/debug/*.c"
  "src/debug/*.h"
  "src/ending/*.c"
  "src/ending/*.h"
  "src/racing/*.c"
  "src/racing/*.h"
  "src/audio/*.c"
  "src/audio/*.h"
  "src/port/*.c"
  "src/port/*.cpp"
  "src/port/*.h"
  "src/mods/*.c"
  "src/mods/*.cpp"
  "src/mods/*.h"
  "assets/code/*.c"
  "courses/*.c"
  "courses/*.h"
  "src/engine/*.cpp"
  "src/engine/*.c"
  "src/engine/*.h"
  "src/enhancements/*.c"
  "src/enhancements/*.cpp"
  "src/enhancements/*.h"
  "Resource.rc")

list(APPEND ALL_FILES ${ALL_FILES_ROOT})

# Exclude specific files from the ALL_FILES list
list(FILTER ALL_FILES EXCLUDE REGEX ".*.inc.c")
list(FILTER ALL_FILES EXCLUDE REGEX "./src/debug/crash_screen_enhancement.c")

if(SPAGHETTIKART_IOS)
  if(NOT EXISTS "${SPAGHETTIPAD_SHELL_DIR}/SpaghettiPadShell.mm")
    message(FATAL_ERROR
      "Set SPAGHETTIPAD_SHELL_DIR to the repo-owned iOS shell directory")
  endif()
  if(NOT EXISTS "${SPAGHETTIPAD_PORT_ARCHIVE}")
    message(FATAL_ERROR
      "Set SPAGHETTIPAD_PORT_ARCHIVE to the ROM-free spaghetti.o2r")
  endif()
endif()

option(SPAGHETTIPAD_RUNTIME_ONLY "Build the experimental iOS module host without game sources" OFF)
if(SPAGHETTIPAD_RUNTIME_ONLY)
  if(NOT SPAGHETTIKART_IOS)
    message(FATAL_ERROR "SPAGHETTIPAD_RUNTIME_ONLY requires an iOS target")
  endif()
  enable_language(OBJC OBJCXX)
  set(ALL_FILES "${SPAGHETTIPAD_SHELL_DIR}/SpaghettiPadGameLoader.mm")
endif()
add_executable(${PROJECT_NAME} ${ALL_FILES})
target_include_directories(
  ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/include
                          ${CMAKE_CURRENT_SOURCE_DIR}/include/assets
                          ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(${PROJECT_NAME} PRIVATE YAML_CPP_STATIC_DEFINE)

if(SPAGHETTIKART_IOS)
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE PLATFORM_IOS=1 __IOS__ MA_NO_DEVICE_IO)
  target_include_directories(${PROJECT_NAME}
    PRIVATE "${SPAGHETTIPAD_SHELL_DIR}")
  set_source_files_properties(src/port/Game.cpp
    PROPERTIES COMPILE_DEFINITIONS main=SDL_main)

  target_sources(${PROJECT_NAME} PRIVATE
    "${SPAGHETTIPAD_SHELL_DIR}/SpaghettiPadShell.mm"
    "${SPAGHETTIPAD_SHELL_DIR}/Assets.xcassets"
    "${SPAGHETTIPAD_PORT_ARCHIVE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.yml"
    "${CMAKE_CURRENT_SOURCE_DIR}/yamls"
    "${CMAKE_CURRENT_SOURCE_DIR}/meta")
  target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreMotion")
  set_source_files_properties(
    "${SPAGHETTIPAD_SHELL_DIR}/Assets.xcassets"
    "${SPAGHETTIPAD_PORT_ARCHIVE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/config.yml"
    "${CMAKE_CURRENT_SOURCE_DIR}/yamls"
    "${CMAKE_CURRENT_SOURCE_DIR}/meta"
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set(CONTROLLER_DB_COMMIT "03d390c37b1342fcb7d8d294cd9621ab9d640bb3")
  set(CONTROLLER_DB_SHA256
    "eb002773dc8a16aa96f9ee2609798e231a9deb60c45e21fbdd4e221c9e8b7d77")
  set(CONTROLLER_DB_PATH "${CMAKE_BINARY_DIR}/gamecontrollerdb.txt")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/mdqinc/SDL_GameControllerDB/${CONTROLLER_DB_COMMIT}/gamecontrollerdb.txt"
    "${CONTROLLER_DB_PATH}"
    EXPECTED_HASH SHA256=${CONTROLLER_DB_SHA256}
    TLS_VERIFY ON)
  target_sources(${PROJECT_NAME} PRIVATE "${CONTROLLER_DB_PATH}")
  set_source_files_properties("${CONTROLLER_DB_PATH}"
    PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set(BUNDLE_ID "com.chrissotraidis.spaghettipad" CACHE STRING
    "iOS application bundle identifier")
  set(DEVELOPMENT_TEAM "" CACHE STRING "Apple development team")
  set(SPAGHETTIPAD_VERSION "0.1.0" CACHE STRING "Application version")
  set(SPAGHETTIPAD_BUILD_NUMBER "1" CACHE STRING "Application build number")
  configure_file(
    "${SPAGHETTIPAD_SHELL_DIR}/Info.plist.in"
    "${CMAKE_BINARY_DIR}/ios/Info.plist"
    @ONLY)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
      OUTPUT_NAME "SpaghettiPad"
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/ios/Info.plist"
      XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUNDLE_ID}"
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
      XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
      XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
      XCODE_ATTRIBUTE_SKIP_INSTALL "NO")
endif()

include(cmake/SetFlags.cmake)

# ##############################################################################
# MSVC runtime library
# ##############################################################################
if(MSVC)
  set_property(
    TARGET ${PROJECT_NAME}
    PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(cmake/dependencies/common.cmake)

include(cmake/dependencies/FindLib.cmake)

# fmt's consteval format-string checking fails to compile under newer
# AppleClang (Xcode 16+) with the spdlog/fmt versions in use (both vcpkg's on
# CI and Torch's bundled copy — see cmake/TorchExternalFixes.cmake for the
# latter):
#   error: call to consteval function 'fmt::basic_format_string<...>' is not
#   a constant expression
# Defining FMT_CONSTEVAL as empty falls back to fmt's pre-C++20 constexpr
# checking, which compiles cleanly with identical runtime behavior. Scoped to
# AppleClang; other platforms and compilers are untouched.
# TODO: Temporary hack. Remove this (plus cmake/TorchExternalFixes.cmake and
# the vcpkg.json fmt/spdlog pins) once the Torch pin is bumped past Torch's
# spdlog/fmt update. Note the bump also needs matching MK64 loader changes,
# see the discussion on PR #712.
if(APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  foreach(FMT_CONSUMER ${PROJECT_NAME} libultraship torch)
    target_compile_definitions(${FMT_CONSUMER} PRIVATE "FMT_CONSTEVAL=")
  endforeach()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")

  nx_generate_nacp(
    ${PROJECT_NAME}.nacp
    NAME
    "${PROJECT_NAME}"
    AUTHOR
    "${PROJECT_TEAM}"
    VERSION
    "${PROJECT_VERSION}")

  nx_create_nro(${PROJECT_NAME} NACP ${PROJECT_NAME}.nacp ICON
                ${CMAKE_CURRENT_SOURCE_DIR}/icon.png)

  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.nro
    DESTINATION .
    COMPONENT ${PROJECT_NAME})
endif()

# ##############################################################################
# Compile and link options
# ##############################################################################


if(NOT SPAGHETTIKART_IOS)
  add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMENT "Copying asset yamls..."
    COMMAND
      ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
      -DDESTINATION_DIR=$<TARGET_FILE_DIR:${PROJECT_NAME}>
      -P ${CMAKE_SOURCE_DIR}/cmake/StageRuntimeFiles.cmake
    VERBATIM)
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch" AND
   NOT SPAGHETTIKART_IOS)
  include(ExternalProject)
  # Torch's pinned spdlog bundles an fmt that trips a consteval error under
  # newer AppleClang (Xcode 16+): "call to consteval function
  # 'fmt::basic_format_string<...>' is not a constant expression". Neutralize
  # FMT_CONSTEVAL inside the Torch sub-build via CMAKE_PROJECT_INCLUDE (Torch's
  # CMakeLists overwrites CMAKE_CXX_FLAGS, so flags can't be injected that
  # way). Fixes the build-macos-arm64 job; the intel runner's older Xcode
  # doesn't hit it.
  # TODO: Temporary hack. Remove the CMAKE_PROJECT_INCLUDE line below (plus
  # cmake/TorchExternalFixes.cmake itself) once the Torch pin is bumped past
  # Torch's spdlog/fmt update, see the discussion on PR #712.
  ExternalProject_Add(TorchExternal
    PREFIX TorchExternal
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/torch
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/torch
               -DCMAKE_PROJECT_INCLUDE=${CMAKE_SOURCE_DIR}/cmake/TorchExternalFixes.cmake
               -DUSE_STANDALONE=ON
               -DBUILD_STORMLIB=${BUILD_STORMLIB}
               -DBUILD_SM64=${BUILD_SM64}
               -DBUILD_MK64=${BUILD_MK64}
               -DBUILD_SF64=${BUILD_SF64}
               -DBUILD_FZERO=${BUILD_FZERO}
               -DBUILD_MARIO_ARTIST=${BUILD_MARIO_ARTIST}
               -DBUILD_NAUDIO=${BUILD_NAUDIO}
  )

  ExternalProject_Get_Property(TorchExternal install_dir)
  if(MSVC)
    set(TORCH_EXECUTABLE
        ${install_dir}/src/TorchExternal-build/$<CONFIGURATION>/torch)
  else()
    set(TORCH_EXECUTABLE ${install_dir}/src/TorchExternal-build/torch)
  endif()

  add_custom_target(
    ExtractAssets
    DEPENDS TorchExternal
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMAND ${TORCH_EXECUTABLE} header -o baserom.us.z64
    COMMAND ${TORCH_EXECUTABLE} o2r baserom.us.z64 --additional-files
            meta/mods.toml
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/mk64.o2r"
            "${CMAKE_BINARY_DIR}/mk64.o2r"
    COMMAND
      ${CMAKE_COMMAND} -DTORCH_EXECUTABLE=${TORCH_EXECUTABLE}
      -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DBINARY_DIR=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/GenerateO2R.cmake
    VERBATIM)

  add_custom_target(
    GenerateO2R
    DEPENDS TorchExternal
    COMMAND
      ${CMAKE_COMMAND} -DTORCH_EXECUTABLE=${TORCH_EXECUTABLE}
      -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -DBINARY_DIR=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/GenerateO2R.cmake
    VERBATIM)

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    install(
      FILES "${CMAKE_BINARY_DIR}/spaghetti.o2r"
      DESTINATION .
      COMPONENT ${PROJECT_NAME})
  endif()

  if(APPLE AND NOT SPAGHETTIKART_IOS)
    add_custom_target(
      CreateOSXIcons
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/macosx
      COMMAND sips -s format icns icon.png --out
              ${CMAKE_BINARY_DIR}/macosx/spaghettikart.icns
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Creating OSX icons ...")
    add_dependencies(${PROJECT_NAME} CreateOSXIcons)
    configure_file("${CMAKE_SOURCE_DIR}/Info.plist"
                   "${CMAKE_BINARY_DIR}/Info.plist" COPYONLY)
  endif()

  set_property(TARGET ${PROJECT_NAME} PROPERTY APPIMAGE_DESKTOP_FILE_TERMINAL
                                               YES)
  set_property(
    TARGET ${PROJECT_NAME} PROPERTY APPIMAGE_DESKTOP_FILE
                                    "${CMAKE_SOURCE_DIR}/SpaghettiKart.desktop")
  set_property(TARGET ${PROJECT_NAME} PROPERTY APPIMAGE_ICON_FILE
                                               "${CMAKE_SOURCE_DIR}/icon.png")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CPACK_GENERATOR "External")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows|NintendoSwitch|CafeOS")
  set(CPACK_GENERATOR "ZIP")
elseif(APPLE)
  set(CPACK_GENERATOR "Bundle")
endif()

set(CPACK_PROJECT_CONFIG_FILE
    ${CMAKE_SOURCE_DIR}/cmake/configure-packaging.cmake)
include(cmake/packaging.cmake)

if(SPAGHETTIPAD_RUNTIME_ONLY)
  include(cmake/SpaghettiPadRuntime.cmake)
endif()
