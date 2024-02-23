set(JACK_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lib/libjack-jackd2-dev_1.9.21~dfsg-3_arm64/include")
list(APPEND JACK_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libjack-jackd2-dev_1.9.21~dfsg-3_arm64/lib/aarch64-linux-gnu/libjack.so.0.1.0")
list(APPEND JACK_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libjack-jackd2-dev_1.9.21~dfsg-3_arm64/lib/aarch64-linux-gnu/libdb-5.3.so")
set(JACK_FOUND True)