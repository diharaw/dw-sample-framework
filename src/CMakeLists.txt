cmake_minimum_required(VERSION 3.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(OpenGL REQUIRED)

set(DFW_SOURCE ${PROJECT_SOURCE_DIR}/src/gl_core_4_5.c
			   ${PROJECT_SOURCE_DIR}/src/main.cpp
			   ${PROJECT_SOURCE_DIR}/external/imgui/imgui.cpp
			   ${PROJECT_SOURCE_DIR}/external/imgui/ImGuizmo.cpp
			   ${PROJECT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
			   ${PROJECT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
			   ${PROJECT_SOURCE_DIR}/external/imgui/examples/opengl3_example/imgui_impl_glfw_gl3.cpp
			   ${PROJECT_SOURCE_DIR}/src/timer.cpp
			   ${PROJECT_SOURCE_DIR}/src/Entity.cpp
			   ${PROJECT_SOURCE_DIR}/src/Scene.cpp
			   ${PROJECT_SOURCE_DIR}/src/Material.cpp
			   ${PROJECT_SOURCE_DIR}/src/Mesh.cpp
			   ${PROJECT_SOURCE_DIR}/src/logger.cpp
			   ${PROJECT_SOURCE_DIR}/src/utility.cpp
			   ${PROJECT_SOURCE_DIR}/src/camera.cpp
			   ${PROJECT_SOURCE_DIR}/src/GLRenderDevice.cpp
			   ${PROJECT_SOURCE_DIR}/src/application.cpp)

set(DFW_HEADERS ${PROJECT_SOURCE_DIR}/src/gl_core_4_5.h
				${PROJECT_SOURCE_DIR}/src/Entity.h
				${PROJECT_SOURCE_DIR}/src/Scene.h
				${PROJECT_SOURCE_DIR}/src/Material.h
				${PROJECT_SOURCE_DIR}/src/Mesh.h
				${PROJECT_SOURCE_DIR}/src/Macros.h
				${PROJECT_SOURCE_DIR}/src/TSMVertex.h
				${PROJECT_SOURCE_DIR}/src/TSMHeaders.h
				${PROJECT_SOURCE_DIR}/external/imgui/imconfig.h
				${PROJECT_SOURCE_DIR}/external/imgui/imgui.h
				${PROJECT_SOURCE_DIR}/external/imgui/imgui_internal.h
				${PROJECT_SOURCE_DIR}/external/imgui/stb_rect_pack.h
				${PROJECT_SOURCE_DIR}/external/imgui/stb_textedit.h
				${PROJECT_SOURCE_DIR}/external/imgui/stb_truetype.h
				${PROJECT_SOURCE_DIR}/external/imgui/ImGuizmo.h
				${PROJECT_SOURCE_DIR}/external/imgui/examples/opengl3_example/imgui_impl_glfw_gl3.h
				${PROJECT_SOURCE_DIR}/src/gfx_descs.h
				${PROJECT_SOURCE_DIR}/src/RenderDevice.h
				${PROJECT_SOURCE_DIR}/src/GLRenderDevice.h
				${PROJECT_SOURCE_DIR}/src/gfx_enums.h
				${PROJECT_SOURCE_DIR}/src/gfx_types.h
				${PROJECT_SOURCE_DIR}/src/gfx_types_gl4.h
				${PROJECT_SOURCE_DIR}/src/camera.h
				${PROJECT_SOURCE_DIR}/src/timer.h
				${PROJECT_SOURCE_DIR}/src/application.h
				${PROJECT_SOURCE_DIR}/src/logger.h
				${PROJECT_SOURCE_DIR}/src/utility.h)

add_executable(DemoFramework ${DFW_HEADERS} ${DFW_SOURCE})				

set_target_properties( DemoFramework
    				   PROPERTIES
    				   ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
    				   LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
    				   RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin" )

set_target_properties(DemoFramework PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

target_link_libraries(DemoFramework ${OPENGL_LIBRARIES})
target_link_libraries(DemoFramework glfw)