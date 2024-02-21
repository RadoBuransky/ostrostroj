# find_package(Jack REQUIRED)
add_executable(ostrostroj main.cpp)
target_link_libraries(ostrostroj -static -static-libgcc -static-libstdc++)
install(TARGETS ostrostroj RUNTIME DESTINATION bin)