cmake_minimum_required(VERSION 3.22)
project(MKNoiseRhythmMachine VERSION 1.0.0 LANGUAGES C CXX)

set(MK_JUCE_SOURCE_DIR "" CACHE PATH "Optional path to a local JUCE checkout")
if(NOT MK_JUCE_SOURCE_DIR)
    get_filename_component(_local_juce "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" ABSOLUTE)
    if(EXISTS "${_local_juce}/CMakeLists.txt")
        set(MK_JUCE_SOURCE_DIR "${_local_juce}")
    endif()
endif()

if(MK_JUCE_SOURCE_DIR)
    add_subdirectory("${MK_JUCE_SOURCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.5
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # The AUv3 app extension is embedded in the Standalone app, which acts as its installable host.
    set(plugin_formats AUv3 Standalone)
elseif(APPLE)
    set(plugin_formats VST3 AUv3 Standalone)
else()
    set(plugin_formats VST3 Standalone)
endif()

juce_add_plugin(MKNoiseRhythmMachine
    COMPANY_NAME "MK"
    BUNDLE_ID "com.mk.noiserhythmmachine"
    PLUGIN_MANUFACTURER_CODE Mkno
    PLUGIN_CODE Nrhm
    FORMATS ${plugin_formats}
    PRODUCT_NAME "MK NOISE RhYTM Machine"
    DESCRIPTION "Three-layer noise instrument with tempo-synced rhythmic gating"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Synth Generator
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationPortrait UIInterfaceOrientationPortraitUpsideDown UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight)

juce_add_binary_data(MKNoiseRhythmMachineAssets
    NAMESPACE AnalogAssets
    HEADER_NAME AnalogAssets.h
    SOURCES
        analog_plugin_svgs/background.svg
        analog_plugin_svgs/dropdown.svg
        analog_plugin_svgs/hslider_bg.svg
        analog_plugin_svgs/hslider_thumb.svg
        analog_plugin_svgs/knob_bg.svg
        analog_plugin_svgs/knob_cap.svg
        analog_plugin_svgs/readout.svg
        analog_plugin_svgs/toggle_off.svg
        analog_plugin_svgs/toggle_on.svg
        analog_plugin_svgs/vslider_bg.svg
        analog_plugin_svgs/vslider_thumb.svg
        analog_plugin_svgs/waveform_display.svg)

target_sources(MKNoiseRhythmMachine PRIVATE
    ThirdParty/Gate12/src/dsp/Pattern.cpp
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PitchShifter.h
    Source/GatePresets.h
    Source/AnalogLookAndFeel.cpp
    Source/AnalogLookAndFeel.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)
target_compile_features(MKNoiseRhythmMachine PRIVATE cxx_std_17)
target_compile_definitions(MKNoiseRhythmMachine PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MKNoiseRhythmMachine PRIVATE
    MKNoiseRhythmMachineAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
target_include_directories(MKNoiseRhythmMachine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Gate12/src/dsp")

if(TARGET MKNoiseRhythmMachine_VST3)
    add_custom_command(TARGET MKNoiseRhythmMachine_VST3 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:MKNoiseRhythmMachine_VST3>/../Resources"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Gate12/LICENSE"
            "$<TARGET_FILE_DIR:MKNoiseRhythmMachine_VST3>/../Resources/Gate12-LICENSE.txt")
endif()

include(CTest)
if(BUILD_TESTING AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
    juce_add_console_app(MKNoiseRhythmMachineTests PRODUCT_NAME "MK Noise Rhythm Machine Tests")
    target_sources(MKNoiseRhythmMachineTests PRIVATE Tests/PluginTests.cpp)
    target_compile_features(MKNoiseRhythmMachineTests PRIVATE cxx_std_17)
    target_link_libraries(MKNoiseRhythmMachineTests PRIVATE
        MKNoiseRhythmMachine
        juce::juce_audio_utils)
    add_test(NAME MKNoiseRhythmMachineTests COMMAND MKNoiseRhythmMachineTests)
    set_tests_properties(MKNoiseRhythmMachineTests PROPERTIES TIMEOUT 30)
endif()
