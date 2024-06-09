add_executable(ostrostroj "")
target_sources(ostrostroj
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

set_target_properties(ostrostroj PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)

add_subdirectory(engine)
add_subdirectory(gfx)
add_subdirectory(project)
add_subdirectory(snd)
add_subdirectory(utils)

find_package(ALSA REQUIRED)
find_package(GPIOD REQUIRED)
target_include_directories(ostrostroj SYSTEM PRIVATE ${ALSA_INCLUDE_DIR}
                                      SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/libsnd/include"
                                      SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/farbot/include"
                                      SYSTEM PRIVATE "${CMAKE_SOURCE_DIR}/lib/spidev-lib/src")
target_link_libraries(ostrostroj PRIVATE spdlog::spdlog_header_only
                                 PRIVATE ${ALSA_LIBRARIES}
                                 PRIVATE ${GPIOD_LIBRARIES}
                                 PRIVATE sndfile
                                 PRIVATE spidev-lib++
                                 PRIVATE samplerate)

target_link_options(ostrostroj PRIVATE "-Wl,--gc-sections")