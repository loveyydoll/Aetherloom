cmake_minimum_required(VERSION 3.22)
# Native Apple Silicon only on macOS, set before compiler detection.
if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Aetherloom macOS architecture")
    if(NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        message(FATAL_ERROR "Aetherloom macOS builds require arm64 (native Apple Silicon).")
    endif()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS")
endif()
project(Aetherloom VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850 GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(JUCE)
if(APPLE)
    set(formats AU Standalone)
    option(AETHERLOOM_BUILD_VST3 "Also build VST3 on macOS" OFF)
    if(AETHERLOOM_BUILD_VST3)
        list(APPEND formats VST3)
    endif()
else()
    set(formats Standalone VST3)
endif()
juce_add_plugin(Aetherloom COMPANY_NAME loveyydoll BUNDLE_ID io.github.loveyydoll.aetherloom
    PLUGIN_MANUFACTURER_CODE Lvyd PLUGIN_CODE Aelm
    FORMATS ${formats} PRODUCT_NAME Aetherloom
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice)
target_sources(Aetherloom PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_compile_definitions(Aetherloom PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(Aetherloom PUBLIC juce::juce_audio_utils juce::juce_dsp
    PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
# Optimise without fast-math: keep finite-value and denormal behaviour predictable.
if(APPLE)
    target_compile_options(Aetherloom PRIVATE $<$<CONFIG:Release>:-O3>)
    target_link_libraries(Aetherloom PRIVATE juce::juce_recommended_lto_flags)
endif()
include(CTest)
if(BUILD_TESTING)
    juce_add_console_app(AetherloomTests PRODUCT_NAME AetherloomTests)
    target_sources(AetherloomTests PRIVATE Tests/SynthTests.cpp)
    target_link_libraries(AetherloomTests PRIVATE Aetherloom)
    add_test(NAME synth COMMAND AetherloomTests)
    add_executable(CharacterDSPTests Tests/CharacterDSPTests.cpp)
    add_test(NAME character_dsp COMMAND CharacterDSPTests)
    juce_add_console_app(AetherloomBenchmark PRODUCT_NAME AetherloomBenchmark)
    target_sources(AetherloomBenchmark PRIVATE Tests/RenderBenchmark.cpp)
    target_link_libraries(AetherloomBenchmark PRIVATE Aetherloom)
endif()
