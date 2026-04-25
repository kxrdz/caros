include(../../../cmake/macros.cmake)
project(${SD_TOOLS_EXECPREFIX}translate)
ADD_SDL2_INCLUDEDIR()
SD_ADD_EXECUTABLE(${PROJECT_NAME}
    app.cpp
    app.h
    linexml.cpp
    main.cpp
)
ADD_SDLIB_LIBRARY(${PROJECT_NAME} portability tgf tgfclient)
SD_INSTALL_FILES(BIN TARGETS ${PROJECT_NAME})
