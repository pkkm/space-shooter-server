cmake_minimum_required (VERSION 2.6)

project ("space-shooter-server")

set(CMAKE_BUILD_TYPE RelWithDebInfo) # Debug, RelWithDebInfo, Release.
#set(CMAKE_VERBOSE_MAKEFILE TRUE)

# Profiling.
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")

# Static linking.
set(build_static FALSE)
if(build_static)
  set(BUILD_SHARED_LIBRARIES OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc")
endif()

# Compilation flags.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --std=c99")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror=implicit-function-declaration")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

# Enable POSIX time functions.
add_definitions(-D_POSIX_C_SOURCE=199309L)

add_subdirectory("src")
