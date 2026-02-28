# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

include($ENV{ARMINO_TOOLS_PATH}/build_tools/cmake/project.cmake)
project(qspi_lcd_display)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
set(CMAKE_ASM_FLAGS ${CMAKE_C_FLAGS})