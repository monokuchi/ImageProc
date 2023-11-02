
cmake_minimum_required(VERSION 3.13)
project(ImageProc)

add_compile_options(-g -O0)

set(SRC_FILES src/imageproc.cpp src/util.cpp src/filter.cpp)

#############################################################

add_executable(imageproc ${SRC_FILES})

target_include_directories(imageproc PUBLIC
                           "${CMAKE_SOURCE_DIR}/src"
                          )
