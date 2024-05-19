target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/holtek16d35a.cpp
        ${CMAKE_CURRENT_LIST_DIR}/holtek16d35a.hpp
        ${CMAKE_CURRENT_LIST_DIR}/unicornhatmini.cpp
        ${CMAKE_CURRENT_LIST_DIR}/unicornhatmini.hpp)

target_include_directories(ostrostroj SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/spidev-lib/src")