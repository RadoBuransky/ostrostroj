add_executable(ostrostroj main.cpp)

target_link_libraries(ostrostroj PRIVATE spdlog::spdlog_header_only)

find_package(JACK REQUIRED)
target_include_directories(ostrostroj PRIVATE ${JACK_INCLUDE_DIRS})
target_link_libraries(ostrostroj PRIVATE ${JACK_LIBRARIES})