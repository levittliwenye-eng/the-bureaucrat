cmake_minimum_required(VERSION 3.22)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if(CMAKE_HOST_APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
        "Minimum supported macOS version")
endif()

project(TheBureaucratPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(THE_BUREAUCRAT_COPY_PLUGIN_AFTER_BUILD
       "Copy built plug-ins into the current user's plug-in folders" OFF)
set(THE_BUREAUCRAT_COMPANY_WEBSITE "" CACHE STRING
    "Customer-facing Disconnec audio website")
set(THE_BUREAUCRAT_COMPANY_EMAIL "" CACHE STRING
    "Customer-facing Disconnec audio support email")

set(THE_BUREAUCRAT_JUCE_TAG "8.0.11" CACHE STRING
    "Pinned JUCE release used when JUCE_DIR is not supplied")

set(JUCE_DIR "" CACHE PATH "Path to a JUCE checkout")
if(NOT JUCE_DIR)
    if(DEFINED ENV{JUCE_DIR})
        set(JUCE_DIR "$ENV{JUCE_DIR}" CACHE PATH "Path to a JUCE checkout" FORCE)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH
            "Path to a JUCE checkout" FORCE)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE/CMakeLists.txt")
        set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH
            "Path to a JUCE checkout" FORCE)
    endif()
endif()

if(JUCE_DIR)
    if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "JUCE_DIR does not point to a JUCE checkout: ${JUCE_DIR}")
    endif()
    message(STATUS "Using local JUCE checkout: ${JUCE_DIR}")
    add_subdirectory("${JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    message(STATUS "Fetching pinned JUCE ${THE_BUREAUCRAT_JUCE_TAG}")
    FetchContent_Declare(JUCE
        URL "https://github.com/juce-framework/JUCE/archive/refs/tags/${THE_BUREAUCRAT_JUCE_TAG}.tar.gz"
        URL_HASH "SHA256=b8c84dbcfe1012bc6e9b7067e6d4a80629d89c7f36792ecbbc7cd0e2354620e8")
    FetchContent_MakeAvailable(JUCE)
endif()

set(THE_BUREAUCRAT_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND THE_BUREAUCRAT_FORMATS AU)
endif()

juce_add_plugin(TheBureaucrat
    COMPANY_NAME "Disconnec audio"
    COMPANY_COPYRIGHT "Copyright 2026 Disconnec audio"
    COMPANY_WEBSITE "${THE_BUREAUCRAT_COMPANY_WEBSITE}"
    COMPANY_EMAIL "${THE_BUREAUCRAT_COMPANY_EMAIL}"
    DESCRIPTION "Saturation and intervention effect by Pravda Audio"
    BUNDLE_ID "audio.disconnec.thebureaucrat"
    PLUGIN_MANUFACTURER_CODE DscA
    PLUGIN_CODE BuCr
    FORMATS ${THE_BUREAUCRAT_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    AU_SANDBOX_SAFE TRUE
    COPY_PLUGIN_AFTER_BUILD ${THE_BUREAUCRAT_COPY_PLUGIN_AFTER_BUILD}
    PRODUCT_NAME "THE BUREAUCRAT")

target_sources(TheBureaucrat
    PRIVATE
        Source/DspEngine.cpp
        Source/DspEngine.h
        Source/ParameterIds.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h)

juce_add_binary_data(TheBureaucratAssets
    SOURCES
        UI/v3/runtime/panel-background-1280.png
        UI/v3/runtime/knob-large.png
        UI/v3/runtime/knob-small.png
        UI/v3/runtime/selector.png
        UI/v3/runtime/plan-lever.png
        UI/v3/runtime/gulag-button.png
        UI/v3/runtime/meter-needle.png
        UI/v3/runtime/lamp-off.png
        UI/v3/runtime/lamp-on.png
        UI/v3/runtime/status-on.png)

target_compile_definitions(TheBureaucrat
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(TheBureaucrat
    PRIVATE
        TheBureaucratAssets
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_link_libraries(TheBureaucrat_Standalone
    PRIVATE
        juce::juce_audio_utils)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME TheBureaucratOpenSourceRepository
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/Tests/OpenSourceRepositoryTests.sh")

    juce_add_console_app(TheBureaucratDspTests
        PRODUCT_NAME "The Bureaucrat DSP Tests")

    target_sources(TheBureaucratDspTests
        PRIVATE
            Tests/DspEngineTests.cpp
            Source/DspEngine.cpp
            Source/DspEngine.h)

    target_include_directories(TheBureaucratDspTests PRIVATE Source)
    target_compile_definitions(TheBureaucratDspTests
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)
    target_link_libraries(TheBureaucratDspTests
        PRIVATE
            juce::juce_audio_basics
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME TheBureaucratDspTests COMMAND TheBureaucratDspTests)

    juce_add_console_app(TheBureaucratUiRender
        PRODUCT_NAME "The Bureaucrat UI Render")

    target_sources(TheBureaucratUiRender
        PRIVATE
            Tests/UiRender.cpp
            Source/DspEngine.cpp
            Source/DspEngine.h
            Source/ParameterIds.h
            Source/PluginEditor.cpp
            Source/PluginEditor.h
            Source/PluginProcessor.cpp
            Source/PluginProcessor.h)
    target_include_directories(TheBureaucratUiRender PRIVATE Source)
    target_compile_definitions(TheBureaucratUiRender
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0)
    target_link_libraries(TheBureaucratUiRender
        PRIVATE
            TheBureaucratAssets
            juce::juce_audio_processors
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    add_test(NAME TheBureaucratUiRender
        COMMAND TheBureaucratUiRender "${CMAKE_BINARY_DIR}/ui-test")

    if(APPLE)
        juce_add_console_app(TheBureaucratPluginHostSmoke
            PRODUCT_NAME "The Bureaucrat Plug-in Host Smoke Test")

        target_sources(TheBureaucratPluginHostSmoke
            PRIVATE
                Tests/PluginHostSmoke.cpp)
        target_compile_definitions(TheBureaucratPluginHostSmoke
            PRIVATE
                JUCE_PLUGINHOST_VST3=1
                JUCE_WEB_BROWSER=0
                JUCE_USE_CURL=0)
        target_link_libraries(TheBureaucratPluginHostSmoke
            PRIVATE
                juce::juce_audio_utils
            PUBLIC
                juce::juce_recommended_config_flags
                juce::juce_recommended_warning_flags)
        add_dependencies(TheBureaucratPluginHostSmoke
            TheBureaucrat_VST3)

        get_target_property(the_bureaucrat_vst3_path
            TheBureaucrat_VST3 JUCE_PLUGIN_ARTEFACT_FILE)
        add_test(NAME TheBureaucratPluginHostSmoke
            COMMAND TheBureaucratPluginHostSmoke
                "${the_bureaucrat_vst3_path}")
    endif()
endif()
