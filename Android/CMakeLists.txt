cmake_minimum_required(VERSION 3.21)
project(PuzzleSolverAndroid VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Quick)
qt_policy(SET QTP0002 NEW)

qt_add_executable(PuzzleSolverAndroid
    main.cpp
    SolverBackend.h
    SolverBackend.cpp
    ../gui_cpp/solver.h
    ../gui_cpp/solver.cpp
    ../gui_cpp/solverworker.h
    ../gui_cpp/solverworker.cpp
)

qt_add_qml_module(PuzzleSolverAndroid
    URI PuzzleSolver
    VERSION 1.0
    QML_FILES
        Main.qml
        BoardCanvas.qml
)

target_include_directories(PuzzleSolverAndroid PRIVATE ../gui_cpp)
target_link_libraries(PuzzleSolverAndroid PRIVATE Qt6::Core Qt6::Quick)

qt_import_plugins(PuzzleSolverAndroid
    INCLUDE_BY_TYPE
        platforms Qt6::QAndroidIntegrationPlugin
    EXCLUDE_BY_TYPE
        qmltooling          # debug/profiling tools — not needed at runtime
        networkinformation  # no network features
        iconengines
        imageformats
        platforminputcontexts
        styles              # Qt Widgets styles — not used (QML Material only)
)

set_target_properties(PuzzleSolverAndroid PROPERTIES
    QT_ANDROID_PACKAGE_NAME              "net.nanamitm.puzzlesolver"
    QT_ANDROID_APP_NAME                  "Puzzle Solver"
    QT_ANDROID_VERSION_NAME              "${PROJECT_VERSION}"
    QT_ANDROID_VERSION_CODE              1
    QT_ANDROID_PACKAGE_SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/android"
    QT_ANDROID_MIN_SDK_VERSION           28
    QT_ANDROID_TARGET_SDK_VERSION        35
    QT_ANDROID_COMPILE_SDK_VERSION       36
    QT_ANDROID_SDK_BUILD_TOOLS_REVISION  36.0.0
)
