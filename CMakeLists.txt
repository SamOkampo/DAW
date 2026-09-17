cmake_minimum_required(VERSION 3.24)
project(FLOWDAW VERSION 0.7.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FLOWDAW_BUILD_APP "Build the X11 bootstrap Studio shell" ON)
option(FLOWDAW_BUILD_TESTS "Build toolkit-independent tests" ON)
option(FLOWDAW_ENABLE_JUCE_RUNTIME "Build the JUCE production runtime" OFF)
option(FLOWDAW_FETCH_JUCE "Fetch pinned JUCE 9.0.2 when the JUCE runtime is enabled" OFF)
option(FLOWDAW_BUILD_JUCE_APP "Build flowdaw-juce production desktop shell" ON)
option(FLOWDAW_BUILD_JUCE_TEST_PLUGIN "Build a real VST3 fixture and end-to-end host test" ON)

add_library(flowdaw_core
    src/MusicalTime.cpp
    src/Midi.cpp
    src/Wav.cpp
    src/Waveform.cpp
    src/NativeDrums.cpp
    src/NativeInstruments.cpp
    src/SampleAnalysis.cpp
    src/SampleEditing.cpp
    src/TimeStretch.cpp
    src/Project.cpp
    src/Serialization.cpp
    src/Undo.cpp
    src/ChopEditing.cpp
    src/Automation.cpp
    src/Export.cpp
    src/PluginHost.cpp
    src/ProductionAssistant.cpp
    src/Workflow.cpp
    src/AppSettings.cpp
    src/SessionRecovery.cpp
    src/PluginSafety.cpp
    src/RuntimeCapabilities.cpp
    src/AudioEngine.cpp
)
target_include_directories(flowdaw_core PUBLIC include)
target_compile_options(flowdaw_core PRIVATE -Wall -Wextra -Wpedantic)

find_library(PORTAUDIO_RUNTIME NAMES portaudio libportaudio.so.2)
if(PORTAUDIO_RUNTIME)
    target_compile_definitions(flowdaw_core PRIVATE FLOWDAW_HAS_PORTAUDIO=1)
    target_link_libraries(flowdaw_core PRIVATE ${PORTAUDIO_RUNTIME})
endif()

if(FLOWDAW_ENABLE_JUCE_RUNTIME)
    if(FLOWDAW_FETCH_JUCE)
        include(FetchContent)
        FetchContent_Declare(JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 9.0.2
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    else()
        find_package(JUCE 9.0.2 CONFIG REQUIRED)
    endif()

    target_compile_definitions(flowdaw_core PUBLIC FLOWDAW_JUCE_RUNTIME=1 FLOWDAW_EXTERNAL_PLUGIN_RUNTIME=1)

    add_library(flowdaw_juce_runtime src/JucePluginBackend.cpp)
    target_include_directories(flowdaw_juce_runtime PUBLIC include)
    target_link_libraries(flowdaw_juce_runtime PUBLIC flowdaw_core juce::juce_audio_processors juce::juce_audio_utils)
    target_compile_definitions(flowdaw_juce_runtime PUBLIC JUCE_PLUGINHOST_VST3=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    if(APPLE)
        target_compile_definitions(flowdaw_juce_runtime PUBLIC JUCE_PLUGINHOST_AU=1)
    else()
        target_compile_definitions(flowdaw_juce_runtime PUBLIC JUCE_PLUGINHOST_AU=0)
    endif()
    target_compile_options(flowdaw_juce_runtime PRIVATE -Wall -Wextra -Wpedantic)

    if(FLOWDAW_BUILD_JUCE_APP)
        juce_add_gui_app(flowdaw-juce PRODUCT_NAME "FLOWDAW")
        target_sources(flowdaw-juce PRIVATE app/juce_main.cpp)
        target_link_libraries(flowdaw-juce PRIVATE flowdaw_juce_runtime juce::juce_gui_extra juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
        target_compile_definitions(flowdaw-juce PRIVATE JUCE_APPLICATION_NAME_STRING="FLOWDAW" JUCE_APPLICATION_VERSION_STRING="0.7.0" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1)
        if(APPLE)
            target_compile_definitions(flowdaw-juce PRIVATE JUCE_PLUGINHOST_AU=1)
        else()
            target_compile_definitions(flowdaw-juce PRIVATE JUCE_PLUGINHOST_AU=0)
        endif()
        install(TARGETS flowdaw-juce
            RUNTIME DESTINATION bin
                PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                            GROUP_READ GROUP_EXECUTE
                            WORLD_READ WORLD_EXECUTE
            BUNDLE DESTINATION .)
    endif()

    if(FLOWDAW_BUILD_TESTS AND FLOWDAW_BUILD_JUCE_TEST_PLUGIN)
        juce_add_plugin(flowdaw_test_plugin
            COMPANY_NAME FLOWDAW
            IS_SYNTH FALSE
            NEEDS_MIDI_INPUT FALSE
            NEEDS_MIDI_OUTPUT FALSE
            IS_MIDI_EFFECT FALSE
            EDITOR_WANTS_KEYBOARD_FOCUS FALSE
            COPY_PLUGIN_AFTER_BUILD FALSE
            PLUGIN_MANUFACTURER_CODE Flwd
            PLUGIN_CODE Ftst
            FORMATS VST3
            PRODUCT_NAME "FLOWDAW Test Plugin")
        target_sources(flowdaw_test_plugin PRIVATE tests/juce_test_plugin.cpp)
        target_link_libraries(flowdaw_test_plugin PRIVATE juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
        target_compile_definitions(flowdaw_test_plugin PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

        add_executable(flowdaw_phase7_juce_tests tests/test_phase7_juce.cpp)
        target_link_libraries(flowdaw_phase7_juce_tests PRIVATE flowdaw_juce_runtime)
        add_dependencies(flowdaw_phase7_juce_tests flowdaw_test_plugin_VST3)
        add_test(NAME flowdaw_phase7_juce_tests COMMAND flowdaw_phase7_juce_tests $<TARGET_FILE:flowdaw_test_plugin_VST3>)
    endif()
endif()

add_executable(flowdaw_stretch_benchmark EXCLUDE_FROM_ALL tools/stretch_benchmark.cpp)
target_link_libraries(flowdaw_stretch_benchmark PRIVATE flowdaw_core)
target_compile_options(flowdaw_stretch_benchmark PRIVATE -Wall -Wextra -Wpedantic)

add_executable(flowdaw-doctor tools/flowdaw_doctor.cpp)
target_link_libraries(flowdaw-doctor PRIVATE flowdaw_core)
target_compile_options(flowdaw-doctor PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS flowdaw-doctor RUNTIME DESTINATION bin)

if(FLOWDAW_BUILD_APP)
    find_package(X11 REQUIRED)
    add_executable(flowdaw app/main_phase5.cpp)
    target_link_libraries(flowdaw PRIVATE flowdaw_core X11::X11)
    target_compile_options(flowdaw PRIVATE -Wall -Wextra -Wpedantic)
    install(TARGETS flowdaw RUNTIME DESTINATION bin)
endif()

if(FLOWDAW_BUILD_TESTS)
    enable_testing()
    add_executable(flowdaw_tests tests/test_main.cpp)
    target_link_libraries(flowdaw_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_tests COMMAND flowdaw_tests)

    add_executable(flowdaw_chop_editing_tests tests/test_chop_editing.cpp)
    target_link_libraries(flowdaw_chop_editing_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_chop_editing_tests COMMAND flowdaw_chop_editing_tests)

    add_executable(flowdaw_sample_editing_tests tests/test_sample_editing.cpp)
    target_link_libraries(flowdaw_sample_editing_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_sample_editing_tests COMMAND flowdaw_sample_editing_tests)

    add_executable(flowdaw_beat_grid_tests tests/test_beat_grid.cpp)
    target_link_libraries(flowdaw_beat_grid_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_beat_grid_tests COMMAND flowdaw_beat_grid_tests)

    add_executable(flowdaw_time_stretch_quality_tests tests/test_time_stretch_quality.cpp)
    target_link_libraries(flowdaw_time_stretch_quality_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_time_stretch_quality_tests COMMAND flowdaw_time_stretch_quality_tests)

    add_executable(flowdaw_phase2_pad_controls_tests tests/test_phase2_pad_controls.cpp)
    target_link_libraries(flowdaw_phase2_pad_controls_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_phase2_pad_controls_tests COMMAND flowdaw_phase2_pad_controls_tests)

    add_executable(flowdaw_phase3_midi_tests tests/test_phase3_midi.cpp)
    target_link_libraries(flowdaw_phase3_midi_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_phase3_midi_tests COMMAND flowdaw_phase3_midi_tests)

    add_executable(flowdaw_phase4_tests tests/test_phase4.cpp)
    target_link_libraries(flowdaw_phase4_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_phase4_tests COMMAND flowdaw_phase4_tests)

    add_executable(flowdaw_phase5_tests tests/test_phase5.cpp)
    target_link_libraries(flowdaw_phase5_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_phase5_tests COMMAND flowdaw_phase5_tests)

    add_executable(flowdaw_phase6_tests tests/test_phase6.cpp)
    target_link_libraries(flowdaw_phase6_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_phase6_tests COMMAND flowdaw_phase6_tests)
endif()

set(CPACK_PACKAGE_NAME "FLOWDAW")
set(CPACK_PACKAGE_VENDOR "FLOWDAW")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "FLOWDAW-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_GENERATOR "TGZ")
include(CPack)
