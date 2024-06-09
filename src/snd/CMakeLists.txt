target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/model_cycles.cpp
        ${CMAKE_CURRENT_LIST_DIR}/model_cycles.hpp)
    
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(alsa)
add_subdirectory(clip)