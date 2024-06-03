target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/bank_pattern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/bank_pattern.hpp
        ${CMAKE_CURRENT_LIST_DIR}/common.cpp
        ${CMAKE_CURRENT_LIST_DIR}/profiler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/profiler.hpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})