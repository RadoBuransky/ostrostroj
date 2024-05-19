target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/clip.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clip.hpp
        ${CMAKE_CURRENT_LIST_DIR}/clipblock.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clipblock.hpp
        ${CMAKE_CURRENT_LIST_DIR}/clipfx.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clipfx.hpp
        ${CMAKE_CURRENT_LIST_DIR}/clipplayer.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clipplayer.hpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})