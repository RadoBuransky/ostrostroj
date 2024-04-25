set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER "C:\\Program Files (x86)\\Arm GNU Toolchain aarch64-none-linux-gnu\\12.2 rel1\\bin\\aarch64-none-linux-gnu-gcc.exe")
set(CMAKE_CXX_COMPILER "C:\\Program Files (x86)\\Arm GNU Toolchain aarch64-none-linux-gnu\\12.2 rel1\\bin\\aarch64-none-linux-gnu-g++.exe")
set(CMAKE_CXX_COMPILER_TARGET "aarch64-none-linux-gnu")
set(PKG_CONFIG_EXECUTABLE "c:\\msys64\\usr\\bin\\pkgconf.exe")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_INIT "-march=armv8.2-a -mtune=cortex-a76 -std=c++20 -Wall -Wextra -Wshadow -Wnon-virtual-dtor -pedantic -fexceptions -fno-threadsafe-statics -fdata-sections -ffunction-sections")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-O2")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-ggdb3 -g -Og")

set(REMOTE_HOSTNAME "ostrostroj.local")