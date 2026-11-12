cmake_minimum_required(VERSION 3.22)

# Set the deployment target before project() initialises the Apple toolchain.
if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING
        "Minimum supported macOS version")
endif()

project(DuganAutomixer
    VERSION 1.0.0
    LANGUAGES C CXX)

add_subdirectory(JUCE)

juce_add_plugin(DuganAutomixer
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Reid Dethorne"
    COMPANY_WEBSITE "https://github.com/reiddethorne"
    BUNDLE_ID "io.github.reiddethorne.duganautomixer"
    PLUGIN_MANUFACTURER_CODE Manu
    PLUGIN_CODE Wagu
    DESCRIPTION "DuganAutomixer"
    PRODUCT_NAME "DuganAutomixer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT
        "Automixer needs microphone access to process live audio."
    FORMATS Standalone)

target_sources(DuganAutomixer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h)

target_compile_features(DuganAutomixer PRIVATE cxx_std_17)

# The existing sources include <JuceHeader.h>. Generate it in the build tree
# so generated JUCE files never need to be stored in the source tree.
juce_generate_juce_header(DuganAutomixer)

target_compile_definitions(DuganAutomixer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(DuganAutomixer
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
