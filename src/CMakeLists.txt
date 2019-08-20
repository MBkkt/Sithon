cmake_minimum_required(VERSION 3.14)
project(Sithon)

set(CMAKE_CXX_STANDARD 17)

add_executable(Sithon
        sithon.cpp
        comparators.cpp
        lexer.cpp
        object.cpp
        object_holder.cpp
        parse.cpp
        statement.cpp
        lexer_test.cpp
        object_holder_test.cpp
        object_test.cpp
        complex_tests.cpp
        parse_test.cpp
        statement_test.cpp)