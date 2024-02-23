include(FeatureSummary)
set_package_properties(jack PROPERTIES
   URL "http://www.jackaudio.org/"
   DESCRIPTION "JACK Audio Connection Kit")

set(JACK_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/jack2/common")
set(JACK_LIBRARIES "${PROJECT_SOURCE_DIR}/lib/jack2")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JACK DEFAULT_MSG JACK_LIBRARIES JACK_INCLUDE_DIRS)
mark_as_advanced(JACK_LIBRARIES JACK_INCLUDE_DIRS JACK_DEFINITIONS)