file(GLOB ostrostroj_SRC "*.h" "*.hpp" "*.cpp")
add_executable(ostrostroj ${ostrostroj_SRC})
set_target_properties(ostrostroj PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

find_package(JACK REQUIRED)
find_package(ALSA REQUIRED)
target_include_directories(ostrostroj PRIVATE ${JACK_INCLUDE_DIR}
                                      PRIVATE ${ALSA_INCLUDE_DIR}
                                      PRIVATE "${CMAKE_SOURCE_DIR}/lib/libsnd/include"
                                      PRIVATE "${CMAKE_SOURCE_DIR}/lib/farbot/include")
target_link_libraries(ostrostroj PRIVATE spdlog::spdlog_header_only
                                 PRIVATE ${JACK_LIBRARIES}
                                 PRIVATE ${ALSA_LIBRARIES}
                                 PRIVATE libremidi
                                 PRIVATE sndfile)

target_link_options(ostrostroj PRIVATE "-Wl,--gc-sections")