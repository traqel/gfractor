# Test executable configuration
cmake_minimum_required(VERSION 3.21)

# Collect test source files
file(GLOB_RECURSE TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Collect plugin source files (needed for testing DSP code)
file(GLOB_RECURSE DSP_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/DSP/*.cpp"
    "${CMAKE_SOURCE_DIR}/Source/DSP/*.h"
)

# Additional source files needed by core tests (not under DSP/)
set(CORE_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/UI/Components/AudioRingBuffer.cpp"
    "${CMAKE_SOURCE_DIR}/Source/UI/Components/PeakHold.cpp"
    "${CMAKE_SOURCE_DIR}/Source/UI/Components/FFTProcessor.cpp"
    "${CMAKE_SOURCE_DIR}/Source/State/PluginState.cpp"
)

# If no test files exist yet, create a dummy test
if(NOT TEST_SOURCES)
    message(WARNING "No test files found in Tests/ directory. Tests will be skipped.")
    return()
endif()

# Create test executable with DSP sources and core sources
add_executable(gFractorTests ${TEST_SOURCES} ${DSP_SOURCES} ${CORE_SOURCES})

# Include directories (access to plugin source code)
target_include_directories(gFractorTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/DSP
        ${CMAKE_SOURCE_DIR}/Source/Parameters
        ${CMAKE_SOURCE_DIR}/Source/State
        ${CMAKE_SOURCE_DIR}/Source/UI
)

# Link against JUCE modules needed for testing
target_link_libraries(gFractorTests
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_data_structures
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# Add same compile definitions as main plugin
target_compile_definitions(gFractorTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

# Platform-specific settings
if(APPLE)
    set_target_properties(gFractorTests PROPERTIES
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "10.13"
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(gFractorTests PRIVATE /W4)
else()
    target_compile_options(gFractorTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Register tests with CTest
add_test(NAME gFractorTests COMMAND gFractorTests)

# Enable CTest verbose output on failure
set_tests_properties(gFractorTests PROPERTIES
    TIMEOUT 300
    FAIL_REGULAR_EXPRESSION "FAILED"
)

message(STATUS "Tests configured: ${CMAKE_CURRENT_SOURCE_DIR}")
