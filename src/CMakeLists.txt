cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if (ENABLE_IMGUI)
	add_definitions(-DDWSF_IMGUI)
endif()

if (USE_VULKAN)
    add_definitions(-DDWSF_VULKAN)
	add_definitions(-DDWSF_VULKAN_RAY_TRACING)
endif()

set (CMAKE_CXX_STANDARD 14)

set(DWSFW_SOURCE ${PROJECT_SOURCE_DIR}/external/imgui/imgui.cpp
			     ${PROJECT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
				 ${PROJECT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
				 ${PROJECT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
			     ${PROJECT_SOURCE_DIR}/src/timer.cpp
			     ${PROJECT_SOURCE_DIR}/src/logger.cpp
				 ${PROJECT_SOURCE_DIR}/src/utility.cpp
				 ${PROJECT_SOURCE_DIR}/src/debug_draw.cpp
				 ${PROJECT_SOURCE_DIR}/src/camera.cpp
				 ${PROJECT_SOURCE_DIR}/src/mesh.cpp
				 ${PROJECT_SOURCE_DIR}/src/material.cpp
				 ${PROJECT_SOURCE_DIR}/src/application.cpp
				 ${PROJECT_SOURCE_DIR}/src/profiler.cpp
				 ${PROJECT_SOURCE_DIR}/src/scene.cpp
				 ${PROJECT_SOURCE_DIR}/src/demo_player.cpp
				 ${PROJECT_SOURCE_DIR}/external/imgui/examples/imgui_impl_glfw.cpp)

set(DWSFW_HEADERS ${PROJECT_SOURCE_DIR}/include/macros.h
				  ${PROJECT_SOURCE_DIR}/external/imgui/imconfig.h
				  ${PROJECT_SOURCE_DIR}/external/imgui/imgui.h
				  ${PROJECT_SOURCE_DIR}/external/imgui/imgui_internal.h
				  ${PROJECT_SOURCE_DIR}/external/imgui/imstb_rectpack.h
				  ${PROJECT_SOURCE_DIR}/external/imgui/imstb_textedit.h
				  ${PROJECT_SOURCE_DIR}/external/imgui/imstb_truetype.h
				  ${PROJECT_SOURCE_DIR}/include/imgui_helpers.h
				  ${PROJECT_SOURCE_DIR}/include/mesh.h
				  ${PROJECT_SOURCE_DIR}/include/debug_draw.h
				  ${PROJECT_SOURCE_DIR}/include/geometry.h
				  ${PROJECT_SOURCE_DIR}/include/material.h
				  ${PROJECT_SOURCE_DIR}/include/camera.h
				  ${PROJECT_SOURCE_DIR}/include/timer.h
				  ${PROJECT_SOURCE_DIR}/include/application.h
				  ${PROJECT_SOURCE_DIR}/include/logger.h
				  ${PROJECT_SOURCE_DIR}/include/utility.h
				  ${PROJECT_SOURCE_DIR}/include/profiler.h
				  ${PROJECT_SOURCE_DIR}/include/scene.h
				  ${PROJECT_SOURCE_DIR}/include/demo_player.h
				  ${PROJECT_SOURCE_DIR}/include/vk.h)

if (USE_VULKAN)
	list(APPEND DWSFW_HEADERS ${PROJECT_SOURCE_DIR}/include/vk.h ${PROJECT_SOURCE_DIR}/external/imgui/examples/imgui_impl_vulkan.h)
	list(APPEND DWSFW_SOURCE ${PROJECT_SOURCE_DIR}/src/vk.cpp ${PROJECT_SOURCE_DIR}/include/vk.h ${PROJECT_SOURCE_DIR}/external/imgui/examples/imgui_impl_vulkan.cpp)
else()
	list(APPEND DWSFW_HEADERS ${PROJECT_SOURCE_DIR}/include/ogl.h ${PROJECT_SOURCE_DIR}/external/imgui/examples/imgui_impl_opengl3.h)
	list(APPEND DWSFW_SOURCE ${PROJECT_SOURCE_DIR}/src/ogl.cpp ${PROJECT_SOURCE_DIR}/include/vk.h ${PROJECT_SOURCE_DIR}/external/imgui/examples/imgui_impl_opengl3.cpp)

	if(NOT EMSCRIPTEN)
		list(APPEND DWSFW_HEADERS ${PROJECT_SOURCE_DIR}/include/glad/glad.h)
		list(APPEND DWSFW_SOURCE ${PROJECT_SOURCE_DIR}/src/glad/glad.c)
	endif()
endif()

if (BUILD_SHARED_LIBRARY)
	add_library(dwSampleFramework SHARED ${DWSFW_HEADERS} ${DWSFW_SOURCE})
else()
	add_library(dwSampleFramework ${DWSFW_HEADERS} ${DWSFW_SOURCE})				
endif()

target_link_libraries(dwSampleFramework assimp)

if(EMSCRIPTEN)
	set_target_properties(dwSampleFramework PROPERTIES LINK_FLAGS "-O3 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s USE_GLFW=3 -s USE_WEBGL2=1")
else()
	target_link_libraries(dwSampleFramework glfw)

	if (USE_VULKAN)
		target_link_libraries(dwSampleFramework ${VULKAN_LIBRARY})
	else()
		target_link_libraries(dwSampleFramework ${OPENGL_LIBRARIES})
	endif()
endif()