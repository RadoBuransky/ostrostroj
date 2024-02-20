cmake_minimum_required(VERSION 3.28.0)
project(ostrostroj VERSION 0.1.0 LANGUAGES CXX)

add_executable(ostrostroj main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
