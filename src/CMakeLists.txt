file(GLOB ostrostroj_SRC "*.h" "*.cpp")
add_executable(ostrostroj ${ostrostroj_SRC})
set_target_properties(ostrostroj PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

find_package(JACK REQUIRED)
target_include_directories(ostrostroj PRIVATE ${JACK_INCLUDE_DIR})
target_link_libraries(ostrostroj PRIVATE spdlog::spdlog_header_only
                                 PRIVATE ${JACK_LIBRARIES}
                                 PRIVATE libremidi)

target_link_options(ostrostroj PRIVATE "-Wl,--gc-sections")