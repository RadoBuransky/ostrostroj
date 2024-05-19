file(GLOB ostrostroj_SRC "*.h" "*.hpp" "*.cpp")
add_executable(ostrostroj ${ostrostroj_SRC})
set_target_properties(ostrostroj PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

find_package(ALSA REQUIRED)
target_include_directories(ostrostroj SYSTEM PRIVATE ${ALSA_INCLUDE_DIR}
                                      SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/libsnd/include"
                                      SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/farbot/include"
                                      SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/spidev-lib/src")
target_link_libraries(ostrostroj PRIVATE spdlog::spdlog_header_only
                                 PRIVATE ${ALSA_LIBRARIES}
                                 PRIVATE sndfile
                                 PRIVATE spidev-lib++)

target_link_options(ostrostroj PRIVATE "-Wl,--gc-sections")