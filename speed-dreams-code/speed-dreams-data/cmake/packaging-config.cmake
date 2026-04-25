set(CPACK_PACKAGE_NAME "speed-dreams-data")
set(CPACK_PACKAGE_VENDOR "The Speed Dreams Team")
set(CPACK_PACKAGE_CONTACT "The Speed Dreams Team <https://forge.a-lec.org/speed-dreams>")
set(CPACK_PACKAGE_VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}${CMAKE_PROJECT_VERSION_TWEAK}")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE all)
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"Free and open source motorsport simulator
Speed Dreams is a free and open source motorsport simulator. Originally a
fork of the TORCS project, it has evolved into a higher level of maturity,
featuring realistic physics with tens of high-quality cars and tracks to
choose from.

Speed Dreams features multiple categories from all racing eras (36GP
and 67GP, Supercars, Long Day Series 1/2, and many more), 4
computer-controlled driver implementations, flexible race configuration
(real-time weather conditions, multi-class races, etc.), as well as a
master server to compare your best lap times against other players.

This package contains the game base assets."
)
