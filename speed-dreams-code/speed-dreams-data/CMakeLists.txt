cmake_minimum_required(VERSION 3.14)
project(speed-dreams-data LANGUAGES NONE VERSION 2.4.2)

# Because of this being a pure CMake project (i.e., no languages),
# this might cause a warning message from CMake since CMAKE_INSTALL_LIBDIR
# cannot be determined. Instead, ${CMAKE_INSTALL_PREFIX}/lib will be used,
# since this is assumed as safe.
# Further reference:
# https://gitlab.kitware.com/cmake/cmake/-/issues/23461
include(GNUInstallDirs)

set(datadir ${CMAKE_INSTALL_DATADIR}/games/speed-dreams-2 CACHE PATH
    "Place where all the static data files should go")

execute_process(
    COMMAND git describe --tags --dirty
    RESULT_VARIABLE result
    OUTPUT_VARIABLE version
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(result EQUAL 0)
    string(FIND "${version}" "-" index)

    if(NOT ${index} EQUAL -1)
        string(SUBSTRING ${version} ${index} -1 CMAKE_PROJECT_VERSION_TWEAK)
    endif()
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(
    cmake/${PROJECT_NAME}.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
    PATH_VARS datadir
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION lib/cmake/${PROJECT_NAME}
)

add_subdirectory(data)

include(cmake/packaging-config.cmake)
include(CPack)
