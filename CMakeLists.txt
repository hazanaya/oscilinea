cmake_minimum_required(VERSION 3.22)

project(SoundShape VERSION 0.0.1 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures" FORCE)
endif()

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "")
set(JUCE_BUILD_EXTRAS   OFF CACHE BOOL "")
add_subdirectory(extern/JUCE)

juce_add_plugin(SoundShape
    COMPANY_NAME                "Hazael Anaya"
    COMPANY_EMAIL               "hazael.anaya@gmail.com"
    PLUGIN_MANUFACTURER_CODE    Hzny
    PLUGIN_CODE                 Sshp
    BUNDLE_ID                   com.hazaelanaya.soundshape
    PLUGIN_NAME                 "Sound Shape"
    PRODUCT_NAME                "Sound Shape"
    FORMATS                     AU VST3 Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             "Fx" "Analyzer"
    AU_MAIN_TYPE                "kAudioUnitType_Effect"
    DESCRIPTION                 "Focused sound-shape visualizer with capture tools")

target_sources(SoundShape PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/dsp/ScopeFeed.h
    src/params/Parameters.cpp
    src/params/Parameters.h
    src/scope/SoundShapeRenderer.cpp
    src/scope/SoundShapeRenderer.h
    src/scope/SoundShapeView.cpp
    src/scope/SoundShapeView.h
    src/ui/Controls.cpp
    src/ui/Controls.h
    src/ui/SystemLookAndFeel.cpp
    src/ui/SystemLookAndFeel.h
    src/video/SourceWindow.cpp
    src/video/SourceWindow.h
    src/video/VideoRecorder.h)

if(APPLE)
    target_sources(SoundShape PRIVATE src/video/VideoRecorder_mac.mm)
    set_source_files_properties(src/video/VideoRecorder_mac.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(SoundShape PRIVATE
        "-framework AVFoundation"
        "-framework CoreMedia"
        "-framework CoreVideo")
else()
    target_sources(SoundShape PRIVATE src/video/VideoRecorder_stub.cpp)
endif()

target_include_directories(SoundShape PRIVATE src)

target_compile_definitions(SoundShape PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(SoundShape
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
