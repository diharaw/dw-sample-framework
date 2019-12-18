cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 14)

if (USE_VULKAN)
    add_definitions(-DDWSF_VULKAN)
endif()

if (APPLE)
    add_executable(sample MACOSX_BUNDLE main.cpp)
    set(MACOSX_BUNDLE_BUNDLE_NAME "com.dihara.sample")
elseif (EMSCRIPTEN)
    message(STATUS "Building for Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_executable(sample ${SAMPLE_SOURCES})
    set_target_properties(sample PROPERTIES LINK_FLAGS "--embed-file ${PROJECT_SOURCE_DIR}/data/teapot.obj@teapot.obj --embed-file ${PROJECT_SOURCE_DIR}/data/default.mtl@default.mtl --embed-file ${PROJECT_SOURCE_DIR}/data/default.png@default.png -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
else()
    add_executable(sample main.cpp)				
endif()

target_link_libraries(sample dwSampleFramework)

set_property(TARGET sample PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")
