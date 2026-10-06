cmake_minimum_required(VERSION 3.16)
project(SolverRegressionTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets)
enable_testing()
add_executable(native_regressions native_regressions.cpp
    ../gui_cpp/solver.cpp ../gui_cpp/boardwidget.cpp
    ../maker/maker_solver.cpp ../maker/polyomino.cpp)
target_include_directories(native_regressions PRIVATE ../gui_cpp ../maker)
target_link_libraries(native_regressions PRIVATE Qt6::Widgets)
add_test(NAME native_regressions COMMAND native_regressions
    "${CMAKE_CURRENT_SOURCE_DIR}/../gui_cpp/pieces")
add_executable(thread_regressions thread_regressions.cpp
    ../gui_cpp/solver.cpp ../gui_cpp/solverworker.cpp
    ../Android/SolverBackend.cpp
    ../maker/searchworker.cpp ../maker/analysisworker.cpp
    ../maker/maker_solver.cpp ../maker/polyomino.cpp)
target_include_directories(thread_regressions PRIVATE ../gui_cpp ../maker ../Android)
target_link_libraries(thread_regressions PRIVATE Qt6::Core)
add_test(NAME thread_regressions COMMAND thread_regressions)
set_tests_properties(native_regressions thread_regressions PROPERTIES TIMEOUT 30)
