cmake_minimum_required(VERSION 3.13)
project(bpt)

set(CMAKE_CXX_STANDARD 14)

add_executable(bpt  BTree.hpp class-integer.hpp exception.hpp class-bint.hpp class-matrix.hpp utility.hpp main.cpp )