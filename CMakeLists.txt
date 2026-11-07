cmake_minimum_required(VERSION 3.25)
project(RiffCalc VERSION 0.3.0 LANGUAGES C CXX)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
include(CTest)

# The same validated JSON is embedded for relocatable, offline execution.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS resources/genres.json)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/resources/genres.json" RiffCalc_GENRES_JSON)
string(LENGTH "${RiffCalc_GENRES_JSON}" json_length)
set(RiffCalc_GENRES_CHUNKS "")
set(offset 0)
while(offset LESS json_length)
    math(EXPR remaining "${json_length} - ${offset}")
    if(remaining GREATER 8000)
        set(remaining 8000)
    endif()
    string(SUBSTRING "${RiffCalc_GENRES_JSON}" ${offset} ${remaining} chunk)
    string(APPEND RiffCalc_GENRES_CHUNKS "R\"RiffCalc_JSON(${chunk})RiffCalc_JSON\",\n")
    math(EXPR offset "${offset} + ${remaining}")
endwhile()
configure_file(src/style/bundled.hpp.in generated/bundled.hpp @ONLY)
add_library(packrat_core
    src/core/music.cpp src/core/harmony.cpp src/core/voicing.cpp
    src/core/generator.cpp src/style/profiles.cpp src/midi/midi.cpp
    src/workstation/workstation.cpp src/workstation/state.cpp src/workstation/arrangement.cpp)
target_include_directories(packrat_core PUBLIC include PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_include_directories(packrat_core SYSTEM PRIVATE third_party)
add_executable(packrat_cli src/cli/main.cpp)
set_target_properties(packrat_cli PROPERTIES OUTPUT_NAME riffcalc)
target_link_libraries(packrat_cli PRIVATE packrat_core)
foreach(target packrat_core packrat_cli)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endforeach()
if(BUILD_TESTING)
    add_executable(packrat_tests tests/tests.cpp)
    target_link_libraries(packrat_tests PRIVATE packrat_core)
    if(MSVC)
        target_compile_options(packrat_tests PRIVATE /W4 /WX /permissive- /EHsc /utf-8)
    else()
        target_compile_options(packrat_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
    add_test(NAME packrat_core COMMAND packrat_tests)
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_test(NAME packrat_cli_midi COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration.py"
        "$<TARGET_FILE:packrat_cli>" "${CMAKE_CURRENT_BINARY_DIR}/test-output")
    add_test(NAME riffcalc_docs COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/docs.py")
    add_test(NAME packrat_vendor_hashes COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_vendor.py")
endif()
install(TARGETS packrat_cli RUNTIME DESTINATION bin)
install(FILES LICENSE NOTICE third_party/nlohmann/LICENSE.MIT
    DESTINATION share/licenses/riffcalc)
install(FILES resources/genres.json DESTINATION share/riffcalc)

add_library(packrat_preview src/audio/preview.cpp)
target_link_libraries(packrat_preview PUBLIC packrat_core)
if(BUILD_TESTING)
    add_executable(packrat_workstation_tests tests/workstation.cpp)
    target_link_libraries(packrat_workstation_tests PRIVATE packrat_preview)
    add_test(NAME packrat_workstation COMMAND packrat_workstation_tests "${CMAKE_CURRENT_BINARY_DIR}/workstation-output")
    add_test(NAME packrat_arrangement_midi COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/workstation_midi.py" "${CMAKE_CURRENT_BINARY_DIR}/workstation-output")
    set_tests_properties(packrat_arrangement_midi PROPERTIES DEPENDS packrat_workstation)
endif()
if(DEFINED PACKRAT_BUILD_APP AND NOT DEFINED RIFFCALC_BUILD_APP)
    set(RIFFCALC_BUILD_APP "${PACKRAT_BUILD_APP}" CACHE BOOL "Build native application")
endif()
option(RIFFCALC_BUILD_APP "Build native standalone and VST3" ON)
if(RIFFCALC_BUILD_APP)
    add_subdirectory(third_party/JUCE EXCLUDE_FROM_ALL)
    juce_add_binary_data(RiffCalcResources SOURCES LICENSE docs/quick-start.md)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        execute_process(COMMAND git -c "safe.directory=${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short HEAD WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_VARIABLE RIFFCALC_REV OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    endif()
    if(NOT RIFFCALC_REV)
        set(RIFFCALC_REV "source-archive")
    endif()
    string(APPEND RIFFCALC_REV " / local build")
    configure_file(src/app/BuildInfo.hpp.in generated/BuildInfo.hpp @ONLY)
    juce_add_plugin(RiffCalc
        COMPANY_NAME "Voidmutt Enterprises"
        COMPANY_WEBSITE "https://github.com/khodges42/RiffCalc" BUNDLE_ID "audio.packrat.workstation"
        PLUGIN_MANUFACTURER_CODE Pkrt PLUGIN_CODE Pkr1
        PRODUCT_NAME "RiffCalc 2000 Pro"
        ICON_BIG resources/branding/riffcalc.png ICON_SMALL resources/branding/riffcalc.png IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Synth FORMATS Standalone VST3)
    set_target_properties(RiffCalc_Standalone PROPERTIES OUTPUT_NAME RiffCalc)
    target_sources(RiffCalc PRIVATE src/app/Processor.cpp src/app/Editor.cpp src/app/EditorLayout.cpp src/app/EditorCommands.cpp src/app/ClassicLook.cpp src/app/About.cpp src/app/NoteSheet.cpp)
    target_include_directories(RiffCalc PUBLIC src/app "${CMAKE_CURRENT_BINARY_DIR}/generated")
    target_compile_definitions(RiffCalc PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_ASIO=0
        JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0
        JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_PLUGINHOST_VST3=1
        JUCE_USE_FLAC=0 JUCE_USE_OGGVORBIS=0 JUCE_USE_MP3AUDIOFORMAT=0
        JUCE_USE_WINDOWS_MEDIA_FORMAT=0)
    target_link_libraries(RiffCalc PRIVATE packrat_preview RiffCalcResources
        juce::juce_audio_utils PUBLIC juce::juce_recommended_config_flags)
    if(MSVC)
        target_compile_options(RiffCalc PRIVATE /utf-8 /permissive- /EHsc)
    endif()
    if(BUILD_TESTING)
        add_executable(packrat_plugin_tests tests/plugin.cpp)
        target_link_libraries(packrat_plugin_tests PRIVATE RiffCalc)
        target_compile_definitions(packrat_plugin_tests PRIVATE RIFFCALC_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        target_include_directories(packrat_plugin_tests PRIVATE $<TARGET_PROPERTY:RiffCalc,INCLUDE_DIRECTORIES>)
        target_compile_definitions(packrat_plugin_tests PRIVATE $<TARGET_PROPERTY:RiffCalc,COMPILE_DEFINITIONS>)
        add_test(NAME packrat_plugin COMMAND packrat_plugin_tests "${CMAKE_CURRENT_BINARY_DIR}/workstation-output")
        add_executable(packrat_host tests/host.cpp)
        target_link_libraries(packrat_host PRIVATE RiffCalc)
        target_include_directories(packrat_host PRIVATE $<TARGET_PROPERTY:RiffCalc,INCLUDE_DIRECTORIES>)
        target_compile_definitions(packrat_host PRIVATE $<TARGET_PROPERTY:RiffCalc,COMPILE_DEFINITIONS>)
        add_dependencies(packrat_host RiffCalc_VST3)
        add_test(NAME packrat_vst3_host COMMAND packrat_host "$<TARGET_FILE_DIR:RiffCalc_VST3>/../.." "${CMAKE_CURRENT_BINARY_DIR}/workstation-output")
    endif()
endif()
