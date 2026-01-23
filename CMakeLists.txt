cmake_minimum_required(VERSION 3.18)

project(OrbitalPan VERSION 0.1.0 LANGUAGES CXX)

# If you have JUCE installed locally, set JUCE_DIR to that path when configuring CMake.
# Example: cmake -B build -S . -DJUCE_DIR=/path/to/JUCE

if (DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 7.0.9
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
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(OrbitalPan)

target_sources(OrbitalPan
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
)

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
