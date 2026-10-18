cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime -> the plugin needs no VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(BoyKickboy VERSION 2.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BOY_BUILD_TOOLS "Build the offline render / screenshot test" ON)

include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(juce)
endif()

# BOY series by sonicsystems. The bundle ID and the codes are what FL Studio stores in projects: never change them
# once released. (The first BOY builds used manufacturer Sprw "Sergio Parowka"; since 2.0 it is Sncs / sonicsystems.)
juce_add_plugin(BoyKickboy
    COMPANY_NAME              "sonicsystems"
    PRODUCT_NAME              "KICKBOY"
    PLUGIN_NAME               "KICKBOY"
    BUNDLE_ID                 "com.sonicsystems.boy.kickboy"
    PLUGIN_MANUFACTURER_CODE  Sncs
    PLUGIN_CODE               Kkby
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Instrument Synth
    FORMATS                   VST3)

set(BOY_SOURCES Source/KickBoy.cpp)
set(BOY_DEFS BOY_NAME="KICKBOY" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_REPORT_APP_USAGE=0)

target_sources(BoyKickboy PRIVATE ${BOY_SOURCES})
target_compile_definitions(BoyKickboy PUBLIC ${BOY_DEFS})
target_link_libraries(BoyKickboy
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- offline render + GUI screenshot (the test): KickboyRender demo.wav gui.png -> RESULT: PASS
if(BOY_BUILD_TOOLS)
    juce_add_console_app(KickboyRender PRODUCT_NAME "KickboyRender")
    target_sources(KickboyRender PRIVATE Tools/RenderDemo.cpp)   # it includes Source/KickBoy.cpp itself
    target_compile_definitions(KickboyRender PRIVATE ${BOY_DEFS})
    target_link_libraries(KickboyRender
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../BOY_shared/boy.cmake)
boy_use(BoyKickboy)
if(TARGET KickboyRender)
    boy_use(KickboyRender)
endif()
