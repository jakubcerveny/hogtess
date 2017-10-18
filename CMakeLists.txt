cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

project(hogtess)

add_definitions(-std=c++11 -ggdb)

if(EXISTS /usr/bin/qmake-qt4)
  # disable Qt5 (default on Ubuntu 13.04)
  set(QT_QMAKE_EXECUTABLE /usr/bin/qmake-qt4)
endif()

# use Qt4 support
find_package(Qt4 REQUIRED)

set(QT_USE_QTOPENGL TRUE)
include(${QT_USE_FILE})

add_definitions(-DGL_GLEXT_PROTOTYPES=1)
find_package(OpenGL 4.0 REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

find_package(GLM REQUIRED)

#find_package(Eigen3 3.2 REQUIRED)
#include_directories(${EIGEN3_INCLUDE_DIR})

set(hogtess_SOURCES
    render.cpp render.hpp
    main.cpp main.hpp
    shader.hpp
)

set(hogtess_MOC_HEADERS
    main.hpp render.hpp)

qt4_wrap_cpp(hogtess_MOC_FILES ${hogtess_MOC_HEADERS})

add_executable(hogtess
    ${hogtess_SOURCES}
    ${hogtess_MOC_FILES}
#    ${hogtess_DATA}
)

target_link_libraries(hogtess
   ${QT_LIBRARIES} ${OPENGL_LIBRARIES}
)
