target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/clipblock.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clipplayer.cpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})