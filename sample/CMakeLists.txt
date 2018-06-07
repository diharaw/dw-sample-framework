cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 11)

if(APPLE)
    add_executable(sample MACOSX_BUNDLE main.cpp)
    set(MACOSX_BUNDLE_BUNDLE_NAME "com.dihara.sample") 
else()
    add_executable(sample main.cpp)				
endif()

target_link_libraries(sample dwSampleFramework)

if(EMSCRIPTEN)
    message(STATUS "Building for Emscripten")
    set_target_properties(sample PROPERTIES SUFFIX ".html")
    set_target_properties(sample PROPERTIES LINK_FLAGS "-s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
endif()