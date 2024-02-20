set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_CXX_COMPILER_TARGET "aarch64-none-linux-gnu")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -std=c++20 -fexceptions -fno-threadsafe-statics -march=armv8-a -mtune=cortex-a53")