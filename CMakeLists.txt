cmake_minimum_required(VERSION 3.21)

# Project configuration
project(gFractor VERSION 1.0.0 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Plugin metadata
set(PLUGIN_NAME "gFractor")
set(PLUGIN_MANUFACTURER "GrowlAudio")
set(PLUGIN_MANUFACTURER_CODE "GrAd")  # 4-character code for manufacturer
set(PLUGIN_CODE "gFrt")                # 4-character code for plugin
set(PLUGIN_AU_ID "com.growlaudio.gfractor")
set(PLUGIN_VERSION "${PROJECT_VERSION}")

# Description
set(PLUGIN_DESCRIPTION "gFractor Audio Effect Plugin")

# Formats to build
set(FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND FORMATS AU)
endif()

# Add JUCE subdirectory (assumes JUCE is in JUCE/ folder via submodule)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found! Please run: git submodule update --init --recursive")
endif()

add_subdirectory(JUCE)

# Collect source files
file(GLOB_RECURSE PLUGIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.h"
)

# Create plugin target
juce_add_plugin(${PLUGIN_NAME}
    COMPANY_NAME "${PLUGIN_MANUFACTURER}"
    PLUGIN_MANUFACTURER_CODE "${PLUGIN_MANUFACTURER_CODE}"
    PLUGIN_CODE "${PLUGIN_CODE}"
    FORMATS ${FORMATS}
    PRODUCT_NAME "${PLUGIN_NAME}"

    # Plugin characteristics
    IS_SYNTH FALSE                          # This is an audio effect, not a synth
    NEEDS_MIDI_INPUT FALSE                  # Set TRUE if you need MIDI input
    NEEDS_MIDI_OUTPUT FALSE                 # Set TRUE if you need MIDI output
    IS_MIDI_EFFECT FALSE                    # Set TRUE for MIDI-only effects
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE       # Set TRUE if editor needs keyboard

    # macOS audio permissions
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "gFractor needs access to your audio input for real-time audio processing."

    # Hardened runtime entitlements
    HARDENED_RUNTIME_ENABLED TRUE
    HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"

    # Copy plugin to standard locations after build
    COPY_PLUGIN_AFTER_BUILD TRUE

    # Plugin identifiers
    BUNDLE_ID "${PLUGIN_AU_ID}"

    # Version
    VERSION "${PLUGIN_VERSION}"

    # Description
    DESCRIPTION "${PLUGIN_DESCRIPTION}"
)

# Add source files to target
target_sources(${PLUGIN_NAME} PRIVATE ${PLUGIN_SOURCES})

# Include directories
target_include_directories(${PLUGIN_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Parameters
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/State
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/UI
)

# Link JUCE modules
target_link_libraries(${PLUGIN_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Compiler definitions
target_compile_definitions(${PLUGIN_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
)

# Platform-specific settings
if(APPLE)
    # macOS deployment target
    set_target_properties(${PLUGIN_NAME} PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.13"
    )
endif()

if(MSVC)
    # Windows-specific compiler flags
    # Note: /W4 is already set by juce_recommended_warning_flags
    target_compile_options(${PLUGIN_NAME} PRIVATE
        /bigobj          # Required for juce_dsp large translation units
        /wd4244          # Suppress conversion warnings from JUCE internals
        /wd4267          # Suppress size_t to int warnings from JUCE internals
        /wd4458          # Suppress member hides class member warnings from JUCE
    )
else()
    # GCC/Clang compiler flags
    target_compile_options(${PLUGIN_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add BinaryData for resources if there are any
file(GLOB_RECURSE RESOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Images/*"
)

if(RESOURCE_FILES)
    juce_add_binary_data(BinaryData
        SOURCES ${RESOURCE_FILES}
    )

    target_link_libraries(${PLUGIN_NAME}
        PRIVATE
            BinaryData
    )
endif()

# Testing support
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(Tests)
endif()

# Installation rules
install(TARGETS ${PLUGIN_NAME}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "gFractor Plugin Build Configuration")
message(STATUS "========================================")
message(STATUS "Plugin Name:        ${PLUGIN_NAME}")
message(STATUS "Company:            ${PLUGIN_MANUFACTURER}")
message(STATUS "Version:            ${PLUGIN_VERSION}")
message(STATUS "Plugin Code:        ${PLUGIN_CODE}")
message(STATUS "Manufacturer Code:  ${PLUGIN_MANUFACTURER_CODE}")
message(STATUS "Formats:            ${FORMATS}")
message(STATUS "Build Type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard:       C++${CMAKE_CXX_STANDARD}")
message(STATUS "Build Testing:      ${BUILD_TESTING}")
message(STATUS "========================================")
