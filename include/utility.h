#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <stdio.h>
#include <ogl.h>

namespace dw
{
namespace utility
{
// Returns the absolute path to the resource. It also resolves the path to the 'Resources' directory is macOS app bundles.
extern std::string path_for_resource(const std::string& resource);

// Returns the absolute path of the executable.
extern std::string executable_path();

// Reads the contents of a text file into an std::string. Returns false if file does not exist.
extern bool read_text(std::string path, std::string& out);

// Reads the specified shader source.
extern bool read_shader(const std::string& path, std::string& out, std::vector<std::string> defines = std::vector<std::string>());

extern bool preprocess_shader(const std::string& path, const std::string& src, std::string& out);

// Removes the filename from a file path.
extern std::string path_without_file(std::string filepath);

// Returns the extension of a given file.
extern std::string file_extension(std::string filepath);

extern std::string file_name_from_path(std::string filepath);

// Queries the current working directory.
extern std::string current_working_directory();

// Changes the current working directory.
extern void change_current_working_directory(std::string path);

#if !defined(DWSF_VULKAN)
// Create compute program
extern bool create_compute_program(const std::string& path, gl::Shader** shader, gl::Program** program, std::vector<std::string> defines = std::vector<std::string>());
#endif

} // namespace utility
} // namespace dw
