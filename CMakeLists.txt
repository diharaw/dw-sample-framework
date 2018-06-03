cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("dwSampleFramework")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(DWSFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")
set(GFX_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/Terminus-GFX/include")
set(GLFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glfw/include")
set(GLM_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glm/glm")
set(STB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/stb")
set(IMGUI_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/imgui")
set(ASSIMP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/assimp/include" "${CMAKE_CURRENT_BINARY_DIR}/external/assimp/include")
set(JSON_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/json/single_include/nlohmann")

set(DW_SAMPLE_FRAMEWORK_INCLUDES ${DWSFW_INCLUDE_DIRS}
                                 ${GFX_INCLUDE_DIRS}
                                 ${GLFW_INCLUDE_DIRS}
                                 ${GLM_INCLUDE_DIRS}
                                 ${STB_INCLUDE_DIRS}
                                 ${IMGUI_INCLUDE_DIRS}
                                 ${ASSIMP_INCLUDE_DIRS}
                                 ${JSON_INCLUDE_DIRS} 
                                 PARENT_SCOPE)

include_directories("${DWSFW_INCLUDE_DIRS}"
                    "${GFX_INCLUDE_DIRS}"
                    "${ASSIMP_INCLUDE_DIRS}"
                    "${GLM_INCLUDE_DIRS}"
                    "${STB_INCLUDE_DIRS}"
                    "${IMGUI_INCLUDE_DIRS}"
                    "${JSON_INCLUDE_DIRS}"
                    "${GLFW_INCLUDE_DIRS}")

add_subdirectory(external/assimp)
add_subdirectory(external/glfw)
add_subdirectory(external/Terminus-GFX)
add_subdirectory(src)
add_subdirectory(sample)