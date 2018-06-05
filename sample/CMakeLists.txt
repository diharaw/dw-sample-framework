cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(APPLE)
    add_executable(sample MACOSX_BUNDLE main.cpp)
    set(MACOSX_BUNDLE_BUNDLE_NAME "com.dihara.sample") 
else()
    add_executable(sample main.cpp)				
endif()

target_link_libraries(sample dwSampleFramework)