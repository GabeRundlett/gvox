# Don't set the MSVC compiler.. This works when opening the folder in Visual Studio
include(${CMAKE_CURRENT_LIST_DIR}/vcvars.cmake)

list(APPEND CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${MSVC_ENV_INCLUDE})
list(REMOVE_DUPLICATES CMAKE_C_STANDARD_INCLUDE_DIRECTORIES)
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_STANDARD_INCLUDE_DIRECTORIES} CACHE STRING "")

list(APPEND CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${MSVC_ENV_INCLUDE})
list(REMOVE_DUPLICATES CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES)
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES} CACHE STRING "")

list(APPEND CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES ${MSVC_ENV_INCLUDE})
list(REMOVE_DUPLICATES CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES)
set(CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_RC_STANDARD_INCLUDE_DIRECTORIES} CACHE STRING "")

list(APPEND LINK_DIRECTORIES $ENV{LIB} $ENV{LIBPATH})
list(REMOVE_DUPLICATES LINK_DIRECTORIES)
set(LINK_DIRECTORIES ${LINK_DIRECTORIES} CACHE STRING "")
link_directories(BEFORE ${LINK_DIRECTORIES})

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER cl)
set(CMAKE_CXX_COMPILER cl)
