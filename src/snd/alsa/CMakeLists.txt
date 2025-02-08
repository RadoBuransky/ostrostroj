target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/alsamidi_processor.cpp
        ${CMAKE_CURRENT_LIST_DIR}/alsamidi.cpp
        ${CMAKE_CURRENT_LIST_DIR}/alsapcm.cpp
        ${CMAKE_CURRENT_LIST_DIR}/alsapcmutils.cpp
        ${CMAKE_CURRENT_LIST_DIR}/volmod_processor.cpp)
    
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})