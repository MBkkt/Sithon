cmake_minimum_required(VERSION 3.14)
project(Mython)

set(CMAKE_CXX_STANDARD 17)

add_executable(Mython
        comparators.cpp
        complex_tests.cpp
        lexer.cpp
        lexer_test.cpp
        mython.cpp
        object.cpp
        object_holder.cpp
        object_holder_test.cpp
        object_test.cpp
        parse.cpp
        parse_test.cpp
        statement.cpp
        statement_test.cpp
        UnitTestsFramework.cpp)