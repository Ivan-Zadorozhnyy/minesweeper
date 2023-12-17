cmake_minimum_required(VERSION 3.26)
set(CMAKE_VERBOSE_MAKEFILE on)
project(untitled8)

set(CMAKE_CXX_STANDARD 17)

set(SFML_DIR "/Users/ivanzadorozhnyy/Downloads/SFML-2.6.1-macOS-clang-arm64/lib")

include_directories("/Users/ivanzadorozhnyy/Downloads/SFML-2.6.1-macOS-clang-arm64/include")

link_directories("/Users/ivanzadorozhnyy/Downloads/SFML-2.6.1-macOS-clang-arm64/lib")

add_executable(cmakeProject main.cpp)

target_link_libraries(cmakeProject sfml-graphics sfml-window sfml-system)
