cmake_minimum_required(VERSION 3.16)
project(ssa-improved VERSION 1.4.0)

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/version.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/resource.rc.in ${CMAKE_CURRENT_BINARY_DIR}/generated/resource.rc @ONLY)

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
    ${CMAKE_CURRENT_BINARY_DIR}/generated/resource.rc
)

#
# HLSL shaders
# - every src/shader/hlsl/*.hlsl is a pixel shader (entry point main) that fxc compiles when the mod is built.
# - bytecode is embedded into the DLL as byte arrays; nothing is compiled while the game runs. *.hlsli files are includes
# - a listing of every shader is written to <build dir>/generated/shaders/<name>.asm.
#
find_program(FXC_EXECUTABLE fxc HINTS "$ENV{WindowsSdkVerBinPath}/x86" "$ENV{WindowsSdkVerBinPath}/x64")
if(NOT FXC_EXECUTABLE)
    # not in a developer prompt: take the newest Windows SDK
    file(GLOB FXC_CANDIDATES "C:/Program Files (x86)/Windows Kits/10/bin/*/x86/fxc.exe")
    list(SORT FXC_CANDIDATES)
    list(REVERSE FXC_CANDIDATES)
    if(FXC_CANDIDATES)
        list(GET FXC_CANDIDATES 0 FXC_EXECUTABLE)
    endif()
endif()
if(NOT FXC_EXECUTABLE)
    message(FATAL_ERROR "fxc.exe (the HLSL compiler of the Windows SDK) was not found. Install the Windows SDK or pass -DFXC_EXECUTABLE=<path to fxc.exe>.")
endif()
message(STATUS "HLSL compiler: ${FXC_EXECUTABLE}")

file(GLOB HLSL_SHADERS CONFIGURE_DEPENDS "src/shader/hlsl/*.hlsl")
file(GLOB HLSL_INCLUDES CONFIGURE_DEPENDS "src/shader/hlsl/*.hlsli")
set(SHADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/shaders)

set(SHADER_BYTECODE "")
foreach(SHADER ${HLSL_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    set(SHADER_BIN ${SHADER_DIR}/${SHADER_NAME}.bin)
    add_custom_command(
        OUTPUT ${SHADER_BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_DIR}
        # ps_3_0, optimisation level 3, real flow control for the loops and branches, row major matrices
        COMMAND ${FXC_EXECUTABLE} /nologo /T ps_3_0 /E main /O3 /Gfp /Zpr /Fo ${SHADER_BIN} /Fc ${SHADER_DIR}/${SHADER_NAME}.asm ${SHADER}
        DEPENDS ${SHADER} ${HLSL_INCLUDES}
        COMMENT "Compiling ${SHADER_NAME}.hlsl"
        VERBATIM
    )
    list(APPEND SHADER_BYTECODE ${SHADER_BIN})
endforeach()

string(REPLACE ";" "|" SHADER_BYTECODE_ARG "${SHADER_BYTECODE}")
set(SHADER_EMBED_CPP ${CMAKE_CURRENT_BINARY_DIR}/generated/compiled_shaders.cpp)
add_custom_command(
    OUTPUT ${SHADER_EMBED_CPP}
    COMMAND ${CMAKE_COMMAND} "-DOUTPUT=${SHADER_EMBED_CPP}" "-DFILES=${SHADER_BYTECODE_ARG}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shaders.cmake
    DEPENDS ${SHADER_BYTECODE} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shaders.cmake
    COMMENT "Embedding compiled shaders"
    VERBATIM
)
# the HLSL files are listed so the IDE shows them, the build system must not compile them itself
set_source_files_properties(${HLSL_SHADERS} ${HLSL_INCLUDES} PROPERTIES HEADER_FILE_ONLY TRUE VS_TOOL_OVERRIDE "None")
target_sources(xinput1_3 PRIVATE ${SHADER_EMBED_CPP} ${HLSL_SHADERS} ${HLSL_INCLUDES})

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