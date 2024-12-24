target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/engineworker.cpp
        ${CMAKE_CURRENT_LIST_DIR}/session.cpp
        ${CMAKE_CURRENT_LIST_DIR}/track.cpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})