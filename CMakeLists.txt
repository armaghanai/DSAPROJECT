cmake_minimum_required(VERSION 3.20)
project(search_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB HEADER_FILES "${CMAKE_SOURCE_DIR}/include/*.hpp")

add_executable(main ${SRC_FILES} ${HEADER_FILES})

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Point to MSYS2 paths explicitly
set(STEMMER_INCLUDE "D:/msys64/ucrt64/include")
set(STEMMER_LIB "D:/msys64/ucrt64/lib/libstemmer.a")

target_include_directories(main PRIVATE ${STEMMER_INCLUDE})
target_link_libraries(main PRIVATE ${STEMMER_LIB})