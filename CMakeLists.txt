cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0143)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("RekordboxQLCLink")
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_ROOT "W:/Program Files/boost_1_90_0")
find_package(Boost 1.90.0 REQUIRED COMPONENTS process filesystem)
link_directories("C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64")

add_executable(RekordboxQLCLink "RekordboxQLCLink/RekordboxQLCLink.cpp"  "RekordboxQLCLink/LinkManager.h" "RekordboxQLCLink/LinkManager.cpp" "RekordboxQLCLink/ControllerMidiReader.cpp" "RekordboxQLCLink/ControllerMidiReader.h" "${PROJECT_SOURCE_DIR}/Dependencies/rtmidi/RtMidi.h" "${PROJECT_SOURCE_DIR}/Dependencies/rtmidi/RtMidi.cpp" "RekordboxQLCLink/MidiClock.h" "RekordboxQLCLink/MidiClock.cpp")
include("${PROJECT_SOURCE_DIR}/Dependencies/link/AbletonLinkConfig.cmake")
include("${BOOST_ROOT}/stage/lib/cmake/boost_process-1.90.0/boost_process-config.cmake")
target_compile_definitions(RekordboxQLCLink PRIVATE __WINDOWS_MM__)
target_include_directories(RekordboxQLCLink PRIVATE "RekordboxQLCLink" ${Boost_INCLUDE_DIRS} "${PROJECT_SOURCE_DIR}/Dependencies/rtmidi")
target_link_libraries(RekordboxQLCLink winmm.lib Ableton::Link Boost::process ${Boost_PROCESS_LIBRARY} Boost::filesystem ${Boost_FILESYSTEM_LIBRARY}) 
