# Single source of truth for the plugin's semantic version.
file(READ "${CMAKE_CURRENT_LIST_DIR}/../VERSION" ORBITALPAN_VERSION)
string(STRIP "${ORBITALPAN_VERSION}" ORBITALPAN_VERSION)
if (NOT ORBITALPAN_VERSION MATCHES "^(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)\\.(0|[1-9][0-9]*)$")
    message(FATAL_ERROR "VERSION must contain one version in MAJOR.MINOR.PATCH format, e.g. 0.2.1")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/../VERSION")
