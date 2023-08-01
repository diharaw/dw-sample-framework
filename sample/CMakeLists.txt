cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CXX_STANDARD 14)

if (ENABLE_IMGUI)
	add_definitions(-DDWSF_IMGUI)
endif()

if (USE_VULKAN)
    add_definitions(-DDWSF_VULKAN)

    set(DWSFW_VK_SAMPLE_SOURCE main_vk.cpp)
    set(DWSFW_VK_RAY_TRACING_SAMPLE_SOURCE main_vk_rt.cpp ${PROJECT_SOURCE_DIR}/extras/ray_traced_scene.cpp)

    set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
 
    set(VULKAN_SHADERS ${PROJECT_SOURCE_DIR}/sample/shaders/mesh.vert
                       ${PROJECT_SOURCE_DIR}/sample/shaders/mesh.frag)

    set(VULKAN_RAY_TRACING_SHADERS ${PROJECT_SOURCE_DIR}/sample/shaders/copy.frag
                                   ${PROJECT_SOURCE_DIR}/sample/shaders/triangle.vert
                                   ${PROJECT_SOURCE_DIR}/sample/shaders/mesh.rgen
                                   ${PROJECT_SOURCE_DIR}/sample/shaders/mesh.rmiss
                                   ${PROJECT_SOURCE_DIR}/sample/shaders/mesh.rchit)

    set(VULKAN_ALL_SHADERS ${VULKAN_SHADERS} ${VULKAN_RAY_TRACING_SHADERS})

    source_group("shaders" FILES  ${VULKAN_SHADERS})
    source_group("shaders" FILES  ${VULKAN_RAY_TRACING_SHADERS})

    foreach(GLSL ${VULKAN_ALL_SHADERS})
        get_filename_component(FILE_NAME ${GLSL} NAME)
        set(SPIRV "${CMAKE_SOURCE_DIR}/bin/$(Configuration)/shaders/${FILE_NAME}.spv")
        add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/bin/$(Configuration)/shaders"
            COMMAND ${GLSL_VALIDATOR} --target-env vulkan1.2 -V ${GLSL} -o ${SPIRV}
            DEPENDS ${GLSL})
        list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    endforeach(GLSL)

    add_custom_target(sample_vk_shaders DEPENDS ${SPIRV_BINARY_FILES})

    if (APPLE)
        add_executable(sample_vk MACOSX_BUNDLE ${DWSFW_VK_SAMPLE_SOURCE})
        set(MACOSX_BUNDLE_BUNDLE_NAME "com.dwsf.sample")

        target_link_libraries(sample_vk dwSampleFramework)

        add_dependencies(sample_vk sample_vk_shaders)
    else()
        add_executable(sample_vk ${DWSFW_VK_SAMPLE_SOURCE} ${VULKAN_SHADERS})	
        add_executable(sample_vk_ray_tracing ${DWSFW_VK_RAY_TRACING_SAMPLE_SOURCE} ${VULKAN_RAY_TRACING_SHADERS})	
        
        target_link_libraries(sample_vk dwSampleFramework)
        target_link_libraries(sample_vk_ray_tracing dwSampleFramework)

        add_dependencies(sample_vk sample_vk_shaders)
        add_dependencies(sample_vk_ray_tracing sample_vk_shaders)

        set_property(TARGET sample_vk PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")
        set_property(TARGET sample_vk_ray_tracing PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")
    endif()
else()
    set(DWSFW_GL_SAMPLE_SOURCE main_gl.cpp)

    if (APPLE)
        add_executable(sample_gl MACOSX_BUNDLE ${DWSFW_GL_SAMPLE_SOURCE})
        set(MACOSX_BUNDLE_BUNDLE_NAME "com.dwsf.sample")
    elseif (EMSCRIPTEN)
        message(STATUS "Building for Emscripten")
        set(CMAKE_EXECUTABLE_SUFFIX ".html")
        add_executable(sample_gl ${DWSFW_GL_SAMPLE_SOURCE})
        set_target_properties(sample_gl PROPERTIES LINK_FLAGS "--embed-file ${PROJECT_SOURCE_DIR}/data/teapot.obj@teapot.obj --embed-file ${PROJECT_SOURCE_DIR}/data/default.mtl@default.mtl --embed-file ${PROJECT_SOURCE_DIR}/data/default.png@default.png -O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 -s USE_WEBGL2=1 -s FULL_ES3=1")
    else()
        add_executable(sample_gl ${DWSFW_GL_SAMPLE_SOURCE})	
        
        set_property(TARGET sample_gl PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")
    endif()

    target_link_libraries(sample_gl dwSampleFramework)
endif()