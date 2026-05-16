cmake_minimum_required(VERSION 3.16)
project(PuzzleSolverGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

# Windows: embed icon via .rc file (gives EXE file icon in Explorer)
if(WIN32)
    set(PLATFORM_SOURCES puzzlesolver.rc)
endif()

qt_add_executable(PuzzleSolverGUI WIN32
    main.cpp
    solver.h solver.cpp
    boardwidget.h boardwidget.cpp
    solverworker.h solverworker.cpp
    mainwindow.h mainwindow.cpp
    resources.qrc
    ${PLATFORM_SOURCES}
)

target_link_libraries(PuzzleSolverGUI PRIVATE Qt6::Widgets Qt6::Svg)
