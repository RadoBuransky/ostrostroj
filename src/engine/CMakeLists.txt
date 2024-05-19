target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/engine.cpp
        ${CMAKE_CURRENT_LIST_DIR}/engine.hpp
        ${CMAKE_CURRENT_LIST_DIR}/engineworker.cpp
        ${CMAKE_CURRENT_LIST_DIR}/engineworker.hpp
        ${CMAKE_CURRENT_LIST_DIR}/track.cpp
        ${CMAKE_CURRENT_LIST_DIR}/track.hpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})