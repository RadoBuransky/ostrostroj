target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/command_controller.cpp
        ${CMAKE_CURRENT_LIST_DIR}/loop_encoders.cpp
        ${CMAKE_CURRENT_LIST_DIR}/midi_encoder.cpp
        ${CMAKE_CURRENT_LIST_DIR}/model_cycles.cpp)    
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})