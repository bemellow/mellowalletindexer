cmake_minimum_required (VERSION 3.0)

enable_language(C)
enable_language(CXX)
find_package(Boost 1.68 REQUIRED COMPONENTS thread)

#-------------------------------------------------------------------------------

project(sqlitepp)

file(GLOB SQLITEPP_SOURCES "sqlitepp/*.c" "sqlitepp/*.cpp")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(sqlitepp STATIC ${SQLITEPP_SOURCES})

#if (CMAKE_COMPILER_IS_GNUCC)
#    target_compile_options(sqlitepp PRIVATE "-w")
#endif()

#-------------------------------------------------------------------------------

project (hash)

file(GLOB LIBHASH_SOURCES "libhash/*.c" "libhash/*.cpp")

include_directories(. ${Boost_INCLUDE_DIRS})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(hash STATIC ${LIBHASH_SOURCES})
#target_link_libraries(hash pthread boost_system boost_filesystem samplerate)

#if (CMAKE_COMPILER_IS_GNUCC)
#    target_compile_options(hash PRIVATE "-fvisibility=hidden")
#endif()

#-------------------------------------------------------------------------------

project (misc)

file(GLOB LIBMISC_SOURCES "common/*.cpp" "common/bech32/*.cpp")

#include_directories(. ${Boost_INCLUDE_DIRS})

#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(misc STATIC ${LIBMISC_SOURCES})
#target_link_libraries(misc pthread boost_system boost_filesystem samplerate)

if (CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(misc PRIVATE "-fvisibility=default")
endif()

#-------------------------------------------------------------------------------

project (btcparser)

file(GLOB LIBBTCPARSER_SOURCES "libbtcparser/*.cpp")

include_directories(. ${Boost_INCLUDE_DIRS})

#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(btcparser STATIC ${LIBBTCPARSER_SOURCES})
#target_link_libraries(btcparser pthread boost_system boost_filesystem samplerate)

#if (CMAKE_COMPILER_IS_GNUCC)
#    target_compile_options(btcparser PRIVATE "-fvisibility=hidden")
#endif()

#-------------------------------------------------------------------------------

project (blockchain_parser)

file(GLOB BLOCKCHAINPARSER_SOURCES "blockchain_parser/*.cpp")

include_directories(. ${Boost_INCLUDE_DIRS})
link_directories(${CMAKE_BINARY_DIR}/lib ${Boost_LIBRARY_DIRS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(blockchain_parser ${BLOCKCHAINPARSER_SOURCES})
target_link_libraries(blockchain_parser btcparser misc hash sqlitepp pthread  
boost_filesystem boost_system dl)

#if (CMAKE_COMPILER_IS_GNUCC)
#    target_compile_options(btcparser PRIVATE "-fvisibility=hidden")
#endif()

#-------------------------------------------------------------------------------

project (btcindex)

file(GLOB LIBBTCINDEX_SOURCES "libbtcindex/*.cpp")

include_directories(. ./libbtcindex ${Boost_INCLUDE_DIRS})
link_directories(${CMAKE_BINARY_DIR}/lib ${Boost_LIBRARY_DIRS})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_POSITION_INDEPENDENT_CODE True)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(btcindex MODULE ${LIBBTCINDEX_SOURCES})
target_link_libraries(btcindex btcparser misc hash sqlitepp pthread  
boost_filesystem boost_system dl)

#if (CMAKE_COMPILER_IS_GNUCC)
#    target_compile_options(btcparser PRIVATE "-fvisibility=hidden")
#endif()

#-------------------------------------------------------------------------------
