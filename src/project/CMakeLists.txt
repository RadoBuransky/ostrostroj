target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/clip.cpp
        ${CMAKE_CURRENT_LIST_DIR}/pattern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/project.cpp
        ${CMAKE_CURRENT_LIST_DIR}/song.cpp
        ${CMAKE_CURRENT_LIST_DIR}/workspace.cpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})