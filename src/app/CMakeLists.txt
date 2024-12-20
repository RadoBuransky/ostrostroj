target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/main_app.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ostrostroj_app.cpp
        ${CMAKE_CURRENT_LIST_DIR}/project_selection.cpp)
target_include_directories(ostrostroj PRIVATE ${CMAKE_CURRENT_LIST_DIR})