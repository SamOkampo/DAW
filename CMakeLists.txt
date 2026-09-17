cmake_minimum_required(VERSION 3.24)
project(FLOWDAW VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FLOWDAW_BUILD_APP "Build the X11 prototype shell" ON)
option(FLOWDAW_BUILD_TESTS "Build tests" ON)

add_library(flowdaw_core
    src/MusicalTime.cpp
    src/Wav.cpp
    src/Waveform.cpp
    src/NativeDrums.cpp
    src/SampleAnalysis.cpp
    src/TimeStretch.cpp
    src/Project.cpp
    src/Serialization.cpp
    src/Undo.cpp
    src/AudioEngine.cpp
)
target_include_directories(flowdaw_core PUBLIC include)
target_compile_options(flowdaw_core PRIVATE -Wall -Wextra -Wpedantic)

# The product backend is JUCE 9.x. This bootstrap backend links directly to the
# PortAudio runtime available in the execution environment, without adding a
# source-level dependency to the core.
find_library(PORTAUDIO_RUNTIME NAMES portaudio libportaudio.so.2)
if(PORTAUDIO_RUNTIME)
    target_compile_definitions(flowdaw_core PRIVATE FLOWDAW_HAS_PORTAUDIO=1)
    target_link_libraries(flowdaw_core PRIVATE ${PORTAUDIO_RUNTIME})
endif()

if(FLOWDAW_BUILD_APP)
    find_package(X11 REQUIRED)
    add_executable(flowdaw app/main.cpp)
    target_link_libraries(flowdaw PRIVATE flowdaw_core X11::X11)
    target_compile_options(flowdaw PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(FLOWDAW_BUILD_TESTS)
    enable_testing()
    add_executable(flowdaw_tests tests/test_main.cpp)
    target_link_libraries(flowdaw_tests PRIVATE flowdaw_core)
    add_test(NAME flowdaw_tests COMMAND flowdaw_tests)
endif()
