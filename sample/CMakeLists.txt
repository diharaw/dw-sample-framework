cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(sample main.cpp)				

target_link_libraries(sample dwSampleFramework)