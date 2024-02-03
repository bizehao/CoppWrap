project(
  libcopp
  VERSION "2.2.0"
  DESCRIPTION "Cross-platform coroutine library in C++ ."
  HOMEPAGE_URL "https://libcopp.atframe.work"
  LANGUAGES C CXX ASM)

include("Func.cmake")

add_library(libcopp STATIC)

set(PROJECT_LIBCOPP_ROOT_INC_DIR ${CMAKE_SOURCE_DIR}/include)

target_include_directories(libcopp PUBLIC
    $<BUILD_INTERFACE:${PROJECT_LIBCOPP_ROOT_INC_DIR}>
    $<INSTALL_INTERFACE:include>
)

file(GLOB_RECURSE COPP_SRC_LIST "coroutine/*.cpp" "utils/*.cpp")

# FCTX_ARCH: arm arm64 i386 mips32 ppc32 ppc64 x86_64
# FCTX_PLATFORM: aapcs ms sysv o32
# FCTX_COFF: elf pe macho xcoff 
# FCTX_ASM_COMPILER: gas masm armasm
# FCTX_ASM_SOURCE_EXTENSION: .S .asm

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64" 
    OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64"
    OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "i386"
    OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
        set(FCTX_ARCH x86_64)
    else()
        set(FCTX_ARCH i386)
    endif()
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    if (CMAKE_CL_64)
        set(FCTX_ARCH arm64)
    else()
        set(FCTX_ARCH arm)
    endif()
else()
    message(FATAL_ERROR "arch ${CMAKE_SYSTEM_PROCESSOR} not supported")
endif()

if (WIN32)
    set(FCTX_PLATFORM ms)
    set(FCTX_COFF pe)
    set(FCTX_ASM_SOURCE_EXTENSION .asm)

    if (MINGW)
        set(CMAKE_ASM_COMPILER as)
        set(FCTX_ASM_COMPILER gas)
        enable_language(ASM)
    else()
        set(FCTX_ASM_COMPILER masm)
        enable_language(ASM_MASM)
		
		set(FCTX_LINKER_OPTIONS /SAFESEH:NO)
    endif()
else()
    set(FCTX_PLATFORM sysv)
    set(FCTX_COFF elf)
    set(FCTX_ASM_SOURCE_EXTENSION .S)
    set(FCTX_ASM_COMPILER gas)
    enable_language(ASM)
endif()

set(FCTX_SOURCE_SUFFIX ${FCTX_ARCH}_${FCTX_PLATFORM}_${FCTX_COFF}_${FCTX_ASM_COMPILER}${FCTX_ASM_SOURCE_EXTENSION})
set(FCTX_SOURCE fcontext/asm/make_${FCTX_SOURCE_SUFFIX} fcontext/asm/jump_${FCTX_SOURCE_SUFFIX} fcontext/asm/ontop_${FCTX_SOURCE_SUFFIX})

target_link_options(libcopp PUBLIC ${FCTX_LINKER_OPTIONS})


# =========== libcopp/src - stack_context ===========
set(PROJECT_LIBCOPP_STACK_CONTEXT_SRC_DIR "stack")
set(PROJECT_LIBCOPP_STACK_ALLOC_SRC_DIR "${PROJECT_LIBCOPP_STACK_CONTEXT_SRC_DIR}/allocator")

# ========== stack information ==========
aux_source_directory("${PROJECT_LIBCOPP_STACK_CONTEXT_SRC_DIR}" SRC_LIST)
list(APPEND COPP_SRC_LIST ${SRC_LIST})

# ========== stack allocator ==========
list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_ALLOC_SRC_DIR}/stack_allocator_memory.cpp")
list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_ALLOC_SRC_DIR}/stack_allocator_malloc.cpp")

include(CheckIncludeFileCXX)
include(CheckIncludeFiles)

check_include_file_cxx(Windows.h PROJECT_LIBCOPP_STACK_ALLOC_WINDOWS)
if(PROJECT_LIBCOPP_STACK_ALLOC_WINDOWS)
  echowithcolor(COLOR GREEN "-- stack allocator: enable windows allocator")
  list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_ALLOC_SRC_DIR}/stack_allocator_windows.cpp")
  list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_CONTEXT_SRC_DIR}/stack_traits/stack_traits_windows.cpp")
  set(LIBCOPP_MACRO_SYS_WIN 1)
endif()

check_include_files("sys/resource.h;sys/time.h;unistd.h;sys/types.h;fcntl.h;sys/mman.h"
                    PROJECT_LIBCOPP_STACK_ALLOC_POSIX)

if(PROJECT_LIBCOPP_STACK_ALLOC_POSIX)
  echowithcolor(COLOR GREEN "-- stack allocator: enable posix allocator")
  list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_ALLOC_SRC_DIR}/stack_allocator_posix.cpp")
  list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_CONTEXT_SRC_DIR}/stack_traits/stack_traits_posix.cpp")
  set(LIBCOPP_MACRO_SYS_POSIX 1)
endif()

if(LIBCOPP_ENABLE_SEGMENTED_STACKS)
  if(NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    echowithcolor(COLOR YELLOW "-- set LIBCOPP_ENABLE_SEGMENTED_STACKS but only gcc and clang support segmented stacks")
    unset(LIBCOPP_ENABLE_SEGMENTED_STACKS)
  elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7.0")
    echowithcolor(COLOR YELLOW
                  "-- set LIBCOPP_ENABLE_SEGMENTED_STACKS but gcc 4.7.0 and upper support segmented stacks")
    unset(LIBCOPP_ENABLE_SEGMENTED_STACKS)
  elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0.0")
    echowithcolor(COLOR YELLOW
                  "-- set LIBCOPP_ENABLE_SEGMENTED_STACKS but clang 5.0.0 and upper support segmented stacks")
    unset(LIBCOPP_ENABLE_SEGMENTED_STACKS)
  else()
    echowithcolor(COLOR GREEN "-- Enable segmented stacks")
    add_definitions(-fsplit-stack)
    set(LIBCOPP_MACRO_USE_SEGMENTED_STACKS 1)
  endif()

  if(LIBCOPP_ENABLE_SEGMENTED_STACKS)
    echowithcolor(COLOR GREEN "-- gcc ${CMAKE_CXX_COMPILER_VERSION} found in unix like system.")
    echowithcolor(COLOR GREEN
                  "-- stack allocator: enable segment allocator, add -fsplit-stack to all program use this library")
    list(APPEND COPP_SRC_LIST "${PROJECT_LIBCOPP_STACK_ALLOC_SRC_DIR}/stack_allocator_split_segment.cpp")
  endif()
endif()

if(LIBCOPP_MACRO_USE_VALGRIND OR LIBCOPP_ENABLE_VALGRIND)
  if(Valgrind_ROOT)
    set(VALGRIND_ROOT ${Valgrind_ROOT})
  endif()
  if(VALGRIND_ROOT)
    set(_VALGRIND_SEARCH_ROOT PATHS ${VALGRIND_ROOT} NO_DEFAULT_PATH)
  endif()
  find_path(
    Valgrind_INCLUDE_DIRS
    NAMES "valgrind/valgrind.h"
    PATH_SUFFIXES include ${_VALGRIND_SEARCH_ROOT})
  if(Valgrind_INCLUDE_DIRS)
    echowithcolor(COLOR GREEN "-- Enable valgrind support(${Valgrind_INCLUDE_DIRS}).")
    set(LIBCOPP_MACRO_USE_VALGRIND 1)
    list(APPEND PROJECT_LIBCOPP_DEP_INC_DIR ${Valgrind_INCLUDE_DIRS})
  else()
    echowithcolor(COLOR RED "-- Can not find valgrind/valgrind.h.")
    message(FATAL_ERROR "Can not find valgrind/valgrind.h")
  endif()
endif()

target_sources(libcopp PRIVATE ${COPP_SRC_LIST} ${FCTX_SOURCE})


set(LIBCOPP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(LIBCOPP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(LIBCOPP_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(LIBCOPP_VERSION "${PROJECT_VERSION}")

# libcopp configure
math(EXPR LIBCOPP_API_LEVEL "1000 * ${PROJECT_VERSION_MAJOR} + ${PROJECT_VERSION_MINOR}" OUTPUT_FORMAT DECIMAL)
set(LIBCOPP_ABI_TAG
    "v${LIBCOPP_API_LEVEL}"
    CACHE STRING "ABI tag for libcopp.")

set(LIBCOPP_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/_generated")
file(MAKE_DIRECTORY "${LIBCOPP_GENERATED_DIR}/include/libcopp/utils/config")
file(MAKE_DIRECTORY "${LIBCOPP_GENERATED_DIR}/temp")
configure_file("${PROJECT_LIBCOPP_ROOT_INC_DIR}/libcopp/utils/config/libcopp_build_features.h.in"
               "${LIBCOPP_GENERATED_DIR}/temp/libcopp_build_features.h" @ONLY)
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBCOPP_GENERATED_DIR}/temp/libcopp_build_features.h"
                        "${LIBCOPP_GENERATED_DIR}/include/libcopp/utils/config")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different "${LIBCOPP_GENERATED_DIR}/temp/libcopp_build_features.h"
                        "${CMAKE_SOURCE_DIR}/include/libcopp/utils/config")