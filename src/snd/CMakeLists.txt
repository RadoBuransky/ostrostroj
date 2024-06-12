target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/midi_note.cpp
        ${CMAKE_CURRENT_LIST_DIR}/saturation.cpp
        ${CMAKE_CURRENT_LIST_DIR}/warp.cpp)
    
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(alsa)
add_subdirectory(clip)