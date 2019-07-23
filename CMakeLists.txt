cmake_minimum_required(VERSION "3.3")

project(mpe-surface VERSION "0.0.1")
set(PROJECT_DISPLAY_NAME "MPE surface")

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(mpe-surface
  "sources/application.cpp"
  "sources/main.cpp"
  "sources/main_window.cpp"
  "sources/touch_mpe_handler.cpp"
  "sources/touch_piano.cpp"
  "resources/resources.qrc")
target_include_directories(mpe-surface
  PRIVATE "sources")
target_link_libraries(mpe-surface
  PRIVATE Qt5::Widgets)
