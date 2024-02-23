include(FeatureSummary)
set_package_properties(jack PROPERTIES
   URL "http://www.jackaudio.org/"
   DESCRIPTION "JACK Audio Connection Kit")

set(JACK_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/libjack-jackd2-dev_1.9.21~dfsg-3_arm64/include")
list(APPEND JACK_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libjack-jackd2-dev_1.9.21~dfsg-3_arm64/lib/aarch64-linux-gnu/libjack.so.0.1.0")
list(APPEND JACK_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/libjack-jackd2-dev_1.9.21~dfsg-3_arm64/lib/aarch64-linux-gnu/libdb-5.3.so")
set(JACK_FOUND True)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JACK DEFAULT_MSG JACK_LIBRARIES JACK_INCLUDE_DIRS)
mark_as_advanced(JACK_LIBRARIES JACK_INCLUDE_DIRS JACK_DEFINITIONS)