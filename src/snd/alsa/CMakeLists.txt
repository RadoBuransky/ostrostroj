target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/alsamidi.cpp
        ${CMAKE_CURRENT_LIST_DIR}/alsapcm.cpp
        ${CMAKE_CURRENT_LIST_DIR}/alsapcmutils.cpp)
    
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})