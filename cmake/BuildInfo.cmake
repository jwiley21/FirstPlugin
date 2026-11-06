# Recomputed on configuration, including when reusing a local build directory.
set(ORBITALPAN_SOURCE_COMMIT "unknown")
set(ORBITALPAN_SOURCE_SHORT "source")
set(ORBITALPAN_SOURCE_MODIFIED "unknown")
find_package(Git QUIET)
if (GIT_FOUND)
    execute_process(COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        OUTPUT_VARIABLE revision OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE revision_result ERROR_QUIET)
    if (revision_result EQUAL 0 AND revision MATCHES "^[0-9a-fA-F]+$")
        set(ORBITALPAN_SOURCE_COMMIT "${revision}")
        string(SUBSTRING "${revision}" 0 7 ORBITALPAN_SOURCE_SHORT)
        execute_process(COMMAND "${GIT_EXECUTABLE}" status --porcelain --untracked-files=normal
            WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
            OUTPUT_VARIABLE changes OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE status_result ERROR_QUIET)
        if (status_result EQUAL 0)
            if (changes STREQUAL "")
                set(ORBITALPAN_SOURCE_MODIFIED "no")
            else()
                set(ORBITALPAN_SOURCE_MODIFIED "yes")
                string(APPEND ORBITALPAN_SOURCE_SHORT "-dirty")
            endif()
        endif()
    endif()
endif()

# CI supplies b<RUN_NUMBER>.<RUN_ATTEMPT>-<short commit>. No generated commit
# or automatic VERSION edit is necessary to distinguish every tester download.
set(ORBITALPAN_BUILD_ID "" CACHE STRING "Optional build identity for tester downloads")
if (ORBITALPAN_BUILD_ID STREQUAL "")
    set(ORBITALPAN_EFFECTIVE_BUILD_ID "local-${ORBITALPAN_SOURCE_SHORT}")
else()
    set(ORBITALPAN_EFFECTIVE_BUILD_ID "${ORBITALPAN_BUILD_ID}")
endif()
if (NOT ORBITALPAN_EFFECTIVE_BUILD_ID MATCHES "^[A-Za-z0-9][A-Za-z0-9._-]*$")
    message(FATAL_ERROR "ORBITALPAN_BUILD_ID may contain only letters, digits, dots, underscores and hyphens")
endif()

set(ORBITALPAN_PACKAGE_PREFIX "OrbitalPan-v${PROJECT_VERSION}-${ORBITALPAN_EFFECTIVE_BUILD_ID}")
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/OrbitalPanBuildInfo.h.in"
    "${PROJECT_BINARY_DIR}/generated/OrbitalPanBuildInfo.h" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/BUILD-INFO.txt.in"
    "${PROJECT_BINARY_DIR}/generated/BUILD-INFO.txt" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/packaging/MAC-TESTER-README.txt"
    "${PROJECT_BINARY_DIR}/generated/READ-ME-FIRST.txt" @ONLY)
file(WRITE "${PROJECT_BINARY_DIR}/generated/package-prefix.txt" "${ORBITALPAN_PACKAGE_PREFIX}\n")
message(STATUS "OrbitalPan ${PROJECT_VERSION} / ${ORBITALPAN_EFFECTIVE_BUILD_ID}")
