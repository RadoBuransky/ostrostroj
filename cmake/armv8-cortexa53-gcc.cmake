set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER "C:\\Program Files (x86)\\Arm GNU Toolchain aarch64-none-linux-gnu\\13.2 Rel1\\bin\\aarch64-none-linux-gnu-gcc.exe")
set(CMAKE_CXX_COMPILER "C:\\Program Files (x86)\\Arm GNU Toolchain aarch64-none-linux-gnu\\13.2 Rel1\\bin\\aarch64-none-linux-gnu-g++.exe")

set(CMAKE_CXX_COMPILER_TARGET "aarch64-none-linux-gnu")
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -std=c++20 -fexceptions -fno-threadsafe-statics -march=armv8-a -mtune=cortex-a53")
