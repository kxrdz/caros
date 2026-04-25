# FreeSOLID - Interference Detection Library.
# Copyright (C) 2025 Xavier Del Campo Romero
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.12)
project(soliditf LANGUAGES C CXX VERSION 2.1.2)
add_subdirectory(libsolid)
add_subdirectory(libmoto)
add_subdirectory(libbroad)
add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
install(DIRECTORY include/SOLID DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/3D DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
set(INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(SOLID_LIBRARY
    ${CMAKE_INSTALL_LIBDIR}/libsolid${CMAKE_SHARED_LIBRARY_SUFFIX})
set(SOLID_BROAD_LIBRARY
    ${CMAKE_INSTALL_LIBDIR}/libbroad${CMAKE_SHARED_LIBRARY_SUFFIX})
set(SOLID_MOTO_LIBRARY
    ${CMAKE_INSTALL_LIBDIR}/libmoto${CMAKE_SHARED_LIBRARY_SUFFIX})
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SOLIDConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    cmake/SOLID.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SOLIDConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SOLID
    PATH_VARS
        INCLUDE_DIR
        SOLID_LIBRARY
        SOLID_BROAD_LIBRARY
        SOLID_MOTO_LIBRARY
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SOLIDConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SOLIDConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SOLID
)
