cmake_minimum_required(VERSION 2.8.3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(hogtess)

#### change this to your MFEM path ####
set(MFEM_PATH ${CMAKE_SOURCE_DIR}/../mfem)

add_definitions(-std=c++11 -ggdb)
include(cmake/file2cpp.cmake)

if(EXISTS /usr/bin/qmake-qt4)
  # disable Qt5 (default on Ubuntu 13.04)
  set(QT_QMAKE_EXECUTABLE /usr/bin/qmake-qt4)
endif()

if (POLICY CMP0072)
  # prefer libGL to libOpenGL (Ubuntu 20.04)
  cmake_policy (SET CMP0072 OLD)
endif(POLICY CMP0072)

# use Qt4 support
find_package(Qt4 REQUIRED)
set(QT_USE_QTOPENGL TRUE)
include(${QT_USE_FILE})

# OpenGL
add_definitions(-DGL_GLEXT_PROTOTYPES=1)
find_package(OpenGL 4.0 REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

# OpenGL Math (GLM)
find_package(GLM REQUIRED)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR}/src)
include_directories(${MFEM_PATH})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

add_subdirectory(src)
