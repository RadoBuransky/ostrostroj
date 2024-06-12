target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/bank_pattern.cpp
        ${CMAKE_CURRENT_LIST_DIR}/common.cpp
        ${CMAKE_CURRENT_LIST_DIR}/profiler.cpp)

target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_precompile_headers(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR}/common.hpp)