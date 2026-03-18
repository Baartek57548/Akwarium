cmake_minimum_required(VERSION 3.20)

project(FirmwareUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(FirmwareUI SHARED
  BitmapFont.cpp
  FakeU8g2.cpp
  FirmwareUI.cpp
  SystemControllerCompat.cpp
  U8g2FontData.cpp
  UIRenderers.cpp
  ../../../firmware/src/AquariumAnimation.cpp
)

target_include_directories(FirmwareUI
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../firmware/src
)

if(MSVC)
  target_compile_definitions(FirmwareUI PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
  target_compile_options(FirmwareUI PRIVATE /W3 /EHsc /utf-8 /wd4244 /wd4305 /wd4100)
else()
  target_compile_options(FirmwareUI PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(FirmwareUI PROPERTIES OUTPUT_NAME "FirmwareUI")
