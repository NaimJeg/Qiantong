cmake_minimum_required(VERSION 3.21)
project(QiantongPortable LANGUAGES CXX)

add_library(QiantongCore STATIC Source/QiantongCore/Private/CoreVersion.cpp)
target_include_directories(QiantongCore PUBLIC Source/QiantongCore/Public)
target_compile_features(QiantongCore PUBLIC cxx_std_20)
set_target_properties(QiantongCore PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(QiantongCore PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(QiantongCore PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

include(CTest)
if(BUILD_TESTING)
    # Negative compilation probe: no accidental Unreal SDK/PCH in this host.
    include(CheckCXXSourceCompiles)
    check_cxx_source_compiles("#include <CoreMinimal.h>\nint main() { return 0; }"
        QIANTONG_CAN_INCLUDE_UNREAL)
    if(QIANTONG_CAN_INCLUDE_UNREAL)
        message(FATAL_ERROR "Standalone compiler can see Unreal headers")
    endif()
    add_executable(CoreSmoke Tests/Core/CoreSmoke.cpp)
    target_link_libraries(CoreSmoke PRIVATE QiantongCore)
    add_test(NAME CoreSmoke COMMAND CoreSmoke)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME ModuleLayout COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Core/ModuleLayoutTests.py")
endif()
