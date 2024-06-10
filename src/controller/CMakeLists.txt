target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/loop_encoders.cpp
        ${CMAKE_CURRENT_LIST_DIR}/loop_encoders.hpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_encoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_encoder.hpp
        ${CMAKE_CURRENT_LIST_DIR}/model_cycles.cpp
        ${CMAKE_CURRENT_LIST_DIR}/model_cycles.hpp)
    
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})