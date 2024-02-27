cmake_minimum_required(VERSION 3.28.0)
project(ostrostroj VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
list(APPEND CMAKE_INCLUDE_PATH "${PROJECT_SOURCE_DIR}/lib/aarch64-none-linux-gnu/include")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules")


add_subdirectory(lib/${CMAKE_CXX_COMPILER_TARGET})
add_subdirectory(lib/spdlog)
set(JACK_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/aarch64-none-linux-gnu/lib/libjack.so")
# add_compile_definitions(LIBREMIDI_JACK=1 LIBREMIDI_HEADER_ONLY=1)
# set_directory_properties(PROPERTIES COMPILE_DEFINITIONS "LIBREMIDI_JACK=1")
set(LIBREMIDI_HEADER_ONLY ON)
add_subdirectory(lib/libremidi)
add_subdirectory(src)

# target_compile_definitions(libremidi PRIVATE "LIBREMIDI_JACK=1;LIBREMIDI_HEADER_ONLY=1")