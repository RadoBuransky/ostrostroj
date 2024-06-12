target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/display.cpp
        ${CMAKE_CURRENT_LIST_DIR}/holtek16d35a.cpp
        ${CMAKE_CURRENT_LIST_DIR}/unicornhatmini.cpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(ostrostroj SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/spidev-lib/src")