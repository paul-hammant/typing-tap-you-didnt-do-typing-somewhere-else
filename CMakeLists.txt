cmake_minimum_required(VERSION 3.24)
project(DeepInputLogger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
set(CMAKE_AUTOMOC ON)

# Platform-specific input backend
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBINPUT REQUIRED libinput)
    pkg_check_modules(LIBUDEV REQUIRED libudev)
    
    add_library(InputBackend
        src/linux/LibinputReader.cpp
        src/linux/LibinputReader.h
    )
    target_include_directories(InputBackend PRIVATE include ${LIBINPUT_INCLUDE_DIRS} ${LIBUDEV_INCLUDE_DIRS})
    target_link_libraries(InputBackend ${LIBINPUT_LIBRARIES} ${LIBUDEV_LIBRARIES})
    target_compile_definitions(InputBackend PRIVATE PLATFORM_LINUX)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(COREFOUNDATION_FRAMEWORK CoreFoundation)
    find_library(COREGRAPHICS_FRAMEWORK CoreGraphics)
    
    add_library(InputBackend
        src/macos/HIDReader.mm
        src/macos/HIDReader.h
    )
    target_include_directories(InputBackend PRIVATE include third_party)
    target_link_libraries(InputBackend ${IOKIT_FRAMEWORK} ${COREFOUNDATION_FRAMEWORK} ${COREGRAPHICS_FRAMEWORK})
    target_compile_definitions(InputBackend PRIVATE PLATFORM_MACOS)
endif()

# Qt UI library
add_library(AppUI
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
)
target_include_directories(AppUI PRIVATE include)
target_link_libraries(AppUI Qt5::Widgets)

# Main executable
add_executable(DeepInputLogger src/main.cpp)
target_include_directories(DeepInputLogger PRIVATE include src)
target_link_libraries(DeepInputLogger Qt5::Widgets InputBackend AppUI)