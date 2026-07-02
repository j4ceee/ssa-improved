cmake_minimum_required(VERSION 3.16)
project(ssa-improved)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 32-bit target - SSA is x86
# configure with: cmake -A Win32 ..
set(CMAKE_GENERATOR_PLATFORM Win32)

# Global defines
add_definitions(-DWIN32 -D_WIN32_WINNT=0x0602)

# Configuration-specific settings
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG -D_DEBUG -D_ITERATOR_DEBUG_LEVEL=0)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

#
# MinHook
#
add_subdirectory(deps/minhook)

#
# meow-hook
#
add_library(meow-hook INTERFACE)

target_include_directories(meow-hook INTERFACE
    deps/meow-hook/include
)

#
# libusb
#
set(LIBUSB_BUILD_SHARED_LIBS OFF)
add_subdirectory(deps/libusb)

#
# imgui
#
add_subdirectory(deps/imgui-cmake)

#
# xinput1_3 proxy DLL (the shim)
#
add_library(xinput1_3 SHARED)

target_compile_definitions(xinput1_3 PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

target_include_directories(xinput1_3 PRIVATE
    src
    deps/minhook/include
    deps/directxtex/DirectXTex          # dds.h (shared internal header)
    deps/directxtex/DDSTextureLoader    # DDSTextureLoader9.h
    deps/directxtex/ScreenGrab          # ScreenGrab9.h
)

file(GLOB_RECURSE SHIM_SOURCES
    "src/*.h"
    "src/*.cpp"
)
target_sources(xinput1_3 PRIVATE
    ${SHIM_SOURCES}
    src/xinput1_3.def
    deps/directxtex/DDSTextureLoader/DDSTextureLoader9.cpp
    deps/directxtex/ScreenGrab/ScreenGrab9.cpp
)

target_link_libraries(xinput1_3 PRIVATE
    minhook
    hid         # for HidD_* function signatures
    setupapi    # for device enumeration if needed later
    usb-1.0
    imgui
    meow-hook
)

if(MSVC)
    set_target_properties(xinput1_3 PROPERTIES
        LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/src/xinput1_3.def"
    )
endif()

# TODO: update path to match your actual Skylanders install
set(SKYLANDERS_DIR "C:/Program Files (x86)/Activision/Skylanders Spyro's Adventure(TM)")

add_custom_command(TARGET xinput1_3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:xinput1_3>
        "${SKYLANDERS_DIR}/xinput1_3.dll"
    COMMENT "Copying xinput1_3.dll to Skylanders directory"
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT xinput1_3)