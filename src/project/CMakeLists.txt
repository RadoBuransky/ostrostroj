target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/clip.cpp
        ${CMAKE_CURRENT_LIST_DIR}/clip.hpp
        ${CMAKE_CURRENT_LIST_DIR}/project.cpp
        ${CMAKE_CURRENT_LIST_DIR}/project.hpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})