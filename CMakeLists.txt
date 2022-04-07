cmake_minimum_required(VERSION 3.21)
project(Final_Project)

set(CMAKE_CXX_STANDARD 14)

add_executable(differentiator differentiator.cpp CppParser.h SyntaxTreeNode.h CppParser.cpp Context.h Context.cpp
        Diff.h Diff.cpp FunctionDiffStorage.h DefaultFunctionDiffStorage.h DefaultFunctionDiffStorage.cpp)

add_custom_command(
    OUTPUT d_function.h
    COMMAND differentiator function.h
    COMMENT "Running generator"
    VERBATIM
)

#INCLUDE_DIRECTORIES("$ENV{EIGEN3_INCLUDE_DIR}")
#find_package (Eigen3 3.4 REQUIRED NO_MODULE)

add_executable(main main.cpp function.h d_function.h)
#target_link_libraries(main Eigen3::Eigen)
