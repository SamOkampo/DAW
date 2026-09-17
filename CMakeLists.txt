cmake_minimum_required(VERSION 3.24)
project(FLOWDAW VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FLOWDAW_BUILD_APP "Build the X11 prototype shell" ON)
option(FLOWDAW_BUILD_TESTS "Build tests" ON)

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
    src/AudioEngine.cpp
)
target_include_directories(flowdaw_core PUBLIC include)
target_compile_options(flowdaw_core PRIVATE -Wall -Wextra -Wpedantic)

find_library(PORTAUDIO_RUNTIME NAMES portaudio libportaudio.so.2)
if(PORTAUDIO_RUNTIME)
    target_compile_definitions(flowdaw_core PRIVATE FLOWDAW_HAS_PORTAUDIO=1)
    target_link_libraries(flowdaw_core PRIVATE ${PORTAUDIO_RUNTIME})
endif()

add_executable(flowdaw_stretch_benchmark EXCLUDE_FROM_ALL tools/stretch_benchmark.cpp)
target_link_libraries(flowdaw_stretch_benchmark PRIVATE flowdaw_core)
target_compile_options(flowdaw_stretch_benchmark PRIVATE -Wall -Wextra -Wpedantic)

if(FLOWDAW_BUILD_APP)
    find_package(X11 REQUIRED)
    add_executable(flowdaw app/main_phase5.cpp)
    target_link_libraries(flowdaw PRIVATE flowdaw_core X11::X11)
    target_compile_options(flowdaw PRIVATE -Wall -Wextra -Wpedantic)
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
endif()
