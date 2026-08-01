cmake_minimum_required(VERSION 3.22)

project(Oscilinea VERSION 0.0.1 LANGUAGES C CXX)

if(APPLE)
    enable_language(OBJC OBJCXX)
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

if(APPLE)
    set(SYPHON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/extern/Syphon-Framework")
    set(SYPHON_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/syphon_include")

    if(NOT EXISTS "${SYPHON_DIR}/SyphonOpenGLServer.m")
        message(FATAL_ERROR "Syphon source is missing. Run: git submodule update --init --recursive")
    endif()

    file(MAKE_DIRECTORY "${SYPHON_INCLUDE_DIR}/Syphon")
    file(GLOB SYPHON_HEADERS CONFIGURE_DEPENDS "${SYPHON_DIR}/*.h")
    foreach(SYPHON_HEADER IN LISTS SYPHON_HEADERS)
        get_filename_component(SYPHON_HEADER_NAME "${SYPHON_HEADER}" NAME)
        configure_file("${SYPHON_HEADER}"
                       "${SYPHON_INCLUDE_DIR}/Syphon/${SYPHON_HEADER_NAME}"
                       COPYONLY)
    endforeach()

    set(SYPHON_OBJC_SOURCES
        "${SYPHON_DIR}/SyphonCFMessageReceiver.m"
        "${SYPHON_DIR}/SyphonCFMessageSender.m"
        "${SYPHON_DIR}/SyphonGLShader.m"
        "${SYPHON_DIR}/SyphonGLVertices.m"
        "${SYPHON_DIR}/SyphonIOSurfaceImageCore.m"
        "${SYPHON_DIR}/SyphonIOSurfaceImageLegacy.m"
        "${SYPHON_DIR}/SyphonImageBase.m"
        "${SYPHON_DIR}/SyphonMessageQueue.m"
        "${SYPHON_DIR}/SyphonMessageReceiver.m"
        "${SYPHON_DIR}/SyphonMessageSender.m"
        "${SYPHON_DIR}/SyphonMessaging.m"
        "${SYPHON_DIR}/SyphonOpenGLImage.m"
        "${SYPHON_DIR}/SyphonOpenGLServer.m"
        "${SYPHON_DIR}/SyphonPrivate.m"
        "${SYPHON_DIR}/SyphonServerBase.m"
        "${SYPHON_DIR}/SyphonServerConnectionManager.m"
        "${SYPHON_DIR}/SyphonServerGLShader.m"
        "${SYPHON_DIR}/SyphonServerGLVertices.m"
        "${SYPHON_DIR}/SyphonServerRendererCoreGL.m"
        "${SYPHON_DIR}/SyphonServerRendererGL.m"
        "${SYPHON_DIR}/SyphonServerRendererLegacyGL.m")
    set(SYPHON_C_SOURCES
        "${SYPHON_DIR}/SyphonCGL.c"
        "${SYPHON_DIR}/SyphonDispatch.c"
        "${SYPHON_DIR}/SyphonOpenGLFunctions.c")

    add_library(SyphonSource STATIC
        ${SYPHON_OBJC_SOURCES}
        ${SYPHON_C_SOURCES})
    set(SYPHON_COMMON_OPTIONS
        -fblocks
        -include "${SYPHON_DIR}/Syphon_Prefix.pch"
        -include libkern/OSAtomic.h
        -include CoreVideo/CoreVideo.h
        -Wno-deprecated-declarations)
    set(SYPHON_OBJC_OPTIONS
        -fobjc-arc
        ${SYPHON_COMMON_OPTIONS})
    set_source_files_properties(${SYPHON_OBJC_SOURCES}
        PROPERTIES COMPILE_OPTIONS "${SYPHON_OBJC_OPTIONS}")
    set_source_files_properties(${SYPHON_C_SOURCES}
        PROPERTIES COMPILE_OPTIONS "${SYPHON_COMMON_OPTIONS}")
    target_compile_definitions(SyphonSource PRIVATE
        GL_SILENCE_DEPRECATION
        SYPHON_CORE_SHARE)
    target_include_directories(SyphonSource PUBLIC
        "${SYPHON_DIR}"
        "${SYPHON_INCLUDE_DIR}")
    target_link_libraries(SyphonSource PUBLIC
        "-framework Cocoa"
        "-framework IOSurface"
        "-framework OpenGL")
endif()

juce_add_plugin(Oscilinea
    COMPANY_NAME                "Haza"
    COMPANY_EMAIL               "hazael.anaya@gmail.com"
    PLUGIN_MANUFACTURER_CODE    Hzny
    PLUGIN_CODE                 Sshp
    BUNDLE_ID                   com.hazaelanaya.oscilinea
    PLUGIN_NAME                 "Oscilinea"
    PRODUCT_NAME                "Oscilinea"
    ICON_BIG                    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icon/oscilinea-1024.png"
    FORMATS                     AU VST3 Standalone
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             "Fx" "Analyzer"
    AU_MAIN_TYPE                "kAudioUnitType_Effect"
    # The standalone wrapper opens an audio input to visualize it. macOS requires
    # a usage string (and, under the Hardened Runtime, the matching entitlement in
    # packaging/entitlements/standalone.entitlements) or it terminates the app the
    # first time it touches the input device. Plug-ins inherit the host's input.
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT  "Oscilinea visualizes live audio from the selected input device."
    DESCRIPTION                 "Focused sound-shape visualizer with capture tools")

target_sources(Oscilinea PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/params/AppearanceDefaults.cpp
    src/params/AppearanceDefaults.h
    src/dsp/AudioCaptureBuffer.h
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
    src/video/SyphonPublisher.h
    src/video/VideoRecorder.h)

if(APPLE)
    target_sources(Oscilinea PRIVATE
        src/video/SyphonPublisher_mac.mm
        src/video/VideoRecorder_mac.mm)
    set_source_files_properties(src/video/SyphonPublisher_mac.mm
                                src/video/VideoRecorder_mac.mm
                                PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_link_libraries(Oscilinea PRIVATE
        SyphonSource
        "-framework AVFoundation"
        "-framework AudioToolbox"
        "-framework Cocoa"
        "-framework CoreMedia"
        "-framework CoreVideo"
        "-framework IOSurface"
        "-framework OpenGL")
else()
    target_sources(Oscilinea PRIVATE
        src/video/SyphonPublisher_stub.cpp
        src/video/VideoRecorder_stub.cpp)
endif()

target_include_directories(Oscilinea PRIVATE src)

target_compile_definitions(Oscilinea PUBLIC
    GL_SILENCE_DEPRECATION
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Oscilinea
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
