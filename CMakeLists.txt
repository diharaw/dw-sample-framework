cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

project("dwSampleFramework")

# Options
set(BUILD_SAMPLES true CACHE BOOL "Build example projects.")
set(BUILD_SHARED_LIBRARY false CACHE BOOL "Build shared library.")
set(ENABLE_CLANG_FORMATTING false CACHE BOOL "Enable clang formatting.")
set(USE_VULKAN false CACHE BOOL "Use Vulkan graphics API.")
set(ENABLE_IMGUI true CACHE BOOL "Enable ImGui.")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(DWSFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/include")
set(DWSFW_EXTRAS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/extras")
set(GLFW_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glfw/include")
set(GLM_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/glm/glm")
set(STB_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/stb")
set(IMGUI_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/imgui")
set(ASSIMP_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/assimp/include" "${CMAKE_CURRENT_BINARY_DIR}/external/assimp/include")
set(JSON_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/json/single_include/nlohmann")
set(VMA_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/external/VulkanMemoryAllocator/include")

if (USE_VULKAN)
    find_package(Vulkan)
else()
    find_package(OpenGL REQUIRED)
endif()

set(DW_SAMPLE_FRAMEWORK_INCLUDES ${DWSFW_INCLUDE_DIRS}
                                 ${DWSFW_EXTRAS_INCLUDE_DIRS}
                                 ${GLFW_INCLUDE_DIRS}
                                 ${GLM_INCLUDE_DIRS}
                                 ${STB_INCLUDE_DIRS}
                                 ${IMGUI_INCLUDE_DIRS}
                                 ${ASSIMP_INCLUDE_DIRS}
                                 ${JSON_INCLUDE_DIRS}
                                 ${VMA_INCLUDE_DIRS}
                                 ${Vulkan_INCLUDE_DIR}
                                 PARENT_SCOPE)

include_directories(${DWSFW_INCLUDE_DIRS}
                    ${DWSFW_EXTRAS_INCLUDE_DIRS}
                    ${GLFW_INCLUDE_DIRS}
                    ${GLM_INCLUDE_DIRS}
                    ${STB_INCLUDE_DIRS}
                    ${IMGUI_INCLUDE_DIRS}
                    ${ASSIMP_INCLUDE_DIRS}
                    ${VMA_INCLUDE_DIRS}
                    ${Vulkan_INCLUDE_DIR}
                    ${JSON_INCLUDE_DIRS})

if (NOT EMSCRIPTEN)
    add_subdirectory(external/glfw)
endif()
                
add_subdirectory(external/assimp)
add_subdirectory(src)

if (BUILD_SAMPLES)
    add_subdirectory(sample)
endif()

if (ENABLE_CLANG_FORMATTING)
    find_program(CLANG_FORMAT_EXE NAMES "clang-format" DOC "Path to clang-format executable")

    file(GLOB_RECURSE DWSF_HEADERS ${PROJECT_SOURCE_DIR}/include/*.h)
    file(GLOB_RECURSE DWSF_SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

    file(GLOB_RECURSE DWSF_SAMPLE_HEADERS ${PROJECT_SOURCE_DIR}/sample/*.h)
    file(GLOB_RECURSE DWSF_SAMPLE_SOURCES ${PROJECT_SOURCE_DIR}/sample/*.cpp)

    if(CLANG_FORMAT_EXE)
        add_custom_target(clang-format-project-files COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${DWSF_HEADERS} ${DWSF_SOURCES} ${DWSF_SAMPLE_HEADERS} ${DWSF_SAMPLE_SOURCES})
    endif()
endif()