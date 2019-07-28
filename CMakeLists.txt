cmake_minimum_required(VERSION "3.3")

project(mpe-surface VERSION "0.0.1")
set(PROJECT_DISPLAY_NAME "MPE surface")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Widgets OpenGL REQUIRED)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(RtMidi)

add_library(nanovg STATIC
  "thirdparty/nanovg/src/nanovg.c")
target_include_directories(nanovg
  PUBLIC "thirdparty/nanovg/src")

add_executable(mpe-surface
  "sources/application.cpp"
  "sources/main.cpp"
  "sources/main_window.cpp"
  "sources/touch_mpe_handler.cpp"
  "sources/touch_piano.cpp"
  "sources/screen_metrics.cpp"
  "sources/style/qmidictlActionBarStyle.cpp"
  "sources/my_nanovg.cpp"
  "resources/resources.qrc")
target_include_directories(mpe-surface
  PRIVATE "sources")
target_link_libraries(mpe-surface
  PRIVATE Qt5::Widgets
  PRIVATE RtMidi
  PRIVATE nanovg)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
target_include_directories(mpe-surface
  PRIVATE ${OPENGL_INCLUDE_DIR})
target_link_libraries(mpe-surface
  PRIVATE ${OPENGL_LIBRARIES})
