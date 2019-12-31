cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 14)

if (USE_VULKAN)
    add_definitions(-DDWSF_VULKAN)

    if (ENABLE_VULKAN_RAY_TRACING)
        set(DWSFW_SAMPLE_SOURCE main_vk_rt.cpp)
    else()
        set(DWSFW_SAMPLE_SOURCE main_vk.cpp)
    endif()

    if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    else()
        set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
    endif()

    set(VULKAN_SHADERS ${PROJECT_SOURCE_DIR}/sample/mesh.vert
                       ${PROJECT_SOURCE_DIR}/sample/mesh.frag
                       ${PROJECT_SOURCE_DIR}/sample/mesh.rgen
                       ${PROJECT_SOURCE_DIR}/sample/mesh.rmiss
                       ${PROJECT_SOURCE_DIR}/sample/mesh.rchit)

    source_group("shaders" FILES  ${VULKAN_SHADERS})
else()
    set(DWSFW_SAMPLE_SOURCE main_gl.cpp)
endif()

if (APPLE)
    add_executable(sample MACOSX_BUNDLE ${DWSFW_SAMPLE_SOURCE})
    set(MACOSX_BUNDLE_BUNDLE_NAME "com.dihara.sample")
elseif (EMSCRIPTEN)
    message(STATUS "Building for Emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_executable(sample ${DWSFW_SAMPLE_SOURCE})
    set_target_properties(sample PROPERTIES LINK_FLAGS "--embed-file ${PROJECT_SOURCE_DIR}/data/teapot.obj@teapot.obj --embed-file ${PROJECT_SOURCE_DIR}/data/default.mtl@default.mtl --embed-file ${PROJECT_SOURCE_DIR}/data/default.png@default.png -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
else()
    if (USE_VULKAN)
        add_executable(sample ${DWSFW_SAMPLE_SOURCE} ${VULKAN_SHADERS})
    else()
        add_executable(sample ${DWSFW_SAMPLE_SOURCE})
    endif()				
endif()

target_link_libraries(sample dwSampleFramework)

if (USE_VULKAN)
    foreach(GLSL ${VULKAN_SHADERS})
        get_filename_component(FILE_NAME ${GLSL} NAME)
        set(SPIRV "${CMAKE_SOURCE_DIR}/bin/$(Configuration)/shaders/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/$(Configuration)/shaders"
            COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(sample_shaders DEPENDS ${SPIRV_BINARY_FILES})

    add_dependencies(sample sample_shaders)
endif()

set_property(TARGET sample PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")
