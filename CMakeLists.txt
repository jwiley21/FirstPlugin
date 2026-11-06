cmake_minimum_required(VERSION 3.22)

include(cmake/ReadVersion.cmake)
# JUCE adds C sources to our targets, so enable C in this top-level scope too.
# Enabling it only inside JUCE leaves Make/Ninja without C compile rules here.
project(OrbitalPan VERSION ${ORBITALPAN_VERSION} LANGUAGES C CXX)
include(cmake/BuildInfo.cmake)

# If you have JUCE installed locally, set JUCE_DIR to that path when configuring CMake.
# Example: cmake -B build -S . -DJUCE_DIR=/path/to/JUCE

if (DEFINED JUCE_DIR)
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.12
    )
    FetchContent_MakeAvailable(juce)
endif()

# Optional: If you have a separate VST3 SDK install, set VST3_SDK_PATH.
# Example: cmake -B build -S . -DVST3_SDK_PATH=/path/to/VST3_SDK
if (DEFINED VST3_SDK_PATH)
    juce_set_vst3_sdk_path(${VST3_SDK_PATH})
endif()

set(PLUGIN_FORMATS VST3)
option(ORBITALPAN_BUILD_STANDALONE "Build standalone target" OFF)
if (ORBITALPAN_BUILD_STANDALONE)
    list(APPEND PLUGIN_FORMATS Standalone)
endif()

option(ORBITALPAN_COPY_PLUGIN_AFTER_BUILD "Install the plugin after building" ON)

juce_add_plugin(OrbitalPan
    COMPANY_NAME "Orbital Audio"
    PRODUCT_NAME "OrbitalPan"
    BUNDLE_ID "com.orbitalaudio.orbitalpan"
    PLUGIN_MANUFACTURER_CODE Orba
    PLUGIN_CODE OrbP
    FORMATS ${PLUGIN_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD ${ORBITALPAN_COPY_PLUGIN_AFTER_BUILD}
)

juce_generate_juce_header(OrbitalPan)
target_include_directories(OrbitalPan PUBLIC "${PROJECT_BINARY_DIR}/generated")

target_sources(OrbitalPan
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/ScopeBuffer.h
        Source/StereoWidener.h
)

option(ORBITALPAN_BUILD_TESTS "Build audio regression tests" OFF)
if (ORBITALPAN_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(OrbitalPanTests PRODUCT_NAME "OrbitalPan Tests")
    target_sources(OrbitalPanTests PRIVATE Tests/AudioTests.cpp)
    target_compile_features(OrbitalPanTests PRIVATE cxx_std_17)
    target_link_libraries(OrbitalPanTests PRIVATE OrbitalPan
        juce::juce_audio_processors juce::juce_dsp juce::juce_gui_extra
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME OrbitalPanAudioTests COMMAND OrbitalPanTests)

    # Load the compiled VST3 through a host, including renamed/extracted packages.
    juce_add_console_app(OrbitalPanPluginLoadTests PRODUCT_NAME "OrbitalPan Plugin Load Tests")
    target_sources(OrbitalPanPluginLoadTests PRIVATE Tests/PluginLoadTests.cpp)
    target_compile_features(OrbitalPanPluginLoadTests PRIVATE cxx_std_17)
    target_compile_definitions(OrbitalPanPluginLoadTests PRIVATE
        JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(OrbitalPanPluginLoadTests PRIVATE
        juce::juce_audio_processors juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    add_dependencies(OrbitalPanPluginLoadTests OrbitalPan_VST3)
    add_test(NAME OrbitalPanPluginLoadTests COMMAND OrbitalPanPluginLoadTests
        "$<TARGET_GENEX_EVAL:OrbitalPan_VST3,$<TARGET_PROPERTY:OrbitalPan_VST3,JUCE_PLUGIN_ARTEFACT_FILE>>" "${PROJECT_VERSION}")
    file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/generated/audio-test-path-$<CONFIG>.txt"
        CONTENT "$<TARGET_FILE:OrbitalPanTests>\n")
    file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/generated/plugin-test-path-$<CONFIG>.txt"
        CONTENT "$<TARGET_FILE:OrbitalPanPluginLoadTests>\n")
endif()

target_compile_features(OrbitalPan PUBLIC cxx_std_17)

# JUCE modules needed for DSP, plugin, and UI
target_compile_definitions(OrbitalPan
    PRIVATE
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(OrbitalPan
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

option(ORBITALPAN_COPY_VST3_TO_COMMON "Copy VST3 to system VST3 folder after build" OFF)
set(ORBITALPAN_VST3_DEST_DIR "" CACHE PATH "Destination VST3 directory for post-build copy")
if (WIN32 AND ORBITALPAN_COPY_VST3_TO_COMMON)
    if (NOT ORBITALPAN_VST3_DEST_DIR)
        message(FATAL_ERROR "Set ORBITALPAN_VST3_DEST_DIR to the target VST3 folder before enabling ORBITALPAN_COPY_VST3_TO_COMMON")
    endif()
    # Copies the full .vst3 bundle directory. Requires write access to Program Files.
    set(VST3_BUNDLE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/OrbitalPan_artefacts/$<CONFIG>/VST3/OrbitalPan.vst3")
    set(VST3_BUNDLE_DEST "${ORBITALPAN_VST3_DEST_DIR}/OrbitalPan.vst3")
    add_custom_command(TARGET OrbitalPan_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${VST3_BUNDLE_SOURCE}" "${VST3_BUNDLE_DEST}"
        COMMENT "Copying OrbitalPan.vst3 to the system VST3 folder (disable with ORBITALPAN_COPY_VST3_TO_COMMON=OFF)"
        VERBATIM
    )
endif()
