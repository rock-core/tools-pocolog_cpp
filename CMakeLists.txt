# CMakeLists.txt has to be located in the project folder and cmake has to be
# executed from 'project/build' with 'cmake ../'.
cmake_minimum_required(VERSION 3.2)
project(pocolog_cpp VERSION 0.1)
find_package(Rock)
set(CMAKE_CXX_STANDARD 17)
rock_init()

option(HANDLE_OROGEN_OPAQUES "whether orogen-generated opaques should be automatically handled. Adds a dependency on RTT" OFF)
rock_standard_layout()

