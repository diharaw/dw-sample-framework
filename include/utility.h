#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cassert>
#include <algorithm>
#include <stdio.h>

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

		// Removes the filename from a file path.
		extern std::string path_without_file(std::string filepath);

		// Returns the extension of a given file.
		extern std::string file_extension(std::string filepath);

		// Queries the current working directory.
		extern std::string current_working_directory();

		// Changes the current working directory.
		extern void change_current_working_directory(std::string path);
	} // namespace utility
} // namespace dw
