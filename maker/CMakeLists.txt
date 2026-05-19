cmake_minimum_required(VERSION 3.16)
project(PuzzleMakerGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

if(MSVC)
    add_compile_options(/O2)
else()
    add_compile_options(-O3)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(MAKER_SOURCES
    main.cpp
    polyomino.h polyomino.cpp
    maker_solver.h maker_solver.cpp
    searchworker.h searchworker.cpp
    analysisworker.h analysisworker.cpp
    makerboardwidget.h makerboardwidget.cpp
    mainwindow.h mainwindow.cpp
)

# Windows exe icon via resource file (.rc + windres/rc.exe)
if(WIN32)
    enable_language(RC)
    list(APPEND MAKER_SOURCES icon_maker.rc)
endif()

qt_add_executable(PuzzleMakerGUI WIN32 ${MAKER_SOURCES})

# PNG for in-app window icon (cross-platform)
qt_add_resources(PuzzleMakerGUI "icons"
    PREFIX "/"
    FILES icon_maker.png
)

target_link_libraries(PuzzleMakerGUI PRIVATE Qt6::Widgets)
