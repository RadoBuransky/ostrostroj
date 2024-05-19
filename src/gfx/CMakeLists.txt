target_sources(ostrostroj PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/unicornhatmini.cpp
    ${CMAKE_CURRENT_LIST_DIR}/unicornhatmini.hpp)

target_include_directories(ostrostroj SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/spidev-lib/src")
target_link_libraries(ostrostroj PRIVATE spidev-lib++)