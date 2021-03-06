add_library(RtMidi STATIC "thirdparty/RtMidi/RtMidi.cpp")
target_include_directories(RtMidi PUBLIC "thirdparty/RtMidi")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_compile_definitions(RtMidi PUBLIC "__LINUX_ALSA__")
  target_link_libraries(RtMidi PUBLIC "asound")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  target_compile_definitions(RtMidi PUBLIC "__WINDOWS_MM__")
  target_link_libraries(RtMidi PUBLIC "winmm")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  target_compile_definitions(RtMidi PUBLIC "__MACOSX_CORE__")
  find_library(COREMIDI_LIBRARY "CoreMIDI")
  find_library(COREAUDIO_LIBRARY "CoreAudio")
  target_link_libraries(RtMidi PUBLIC "${COREMIDI_LIBRARY}" "${COREAUDIO_LIBRARY}")
endif()

find_package(Threads REQUIRED)
target_link_libraries(RtMidi PUBLIC ${CMAKE_THREAD_LIBS_INIT})
