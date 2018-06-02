#include "utility.h"

#include <fstream>
#include <iostream>

#ifdef WIN32
#include <Windows.h>
#include <direct.h>
#define GetCurrentDir _getcwd
#define ChangeWorkingDir _chdir
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#define ChangeWorkingDir chdir
#endif

namespace dw
{
	namespace utility
	{
		static std::string g_exe_path = "";

		std::string path_for_resource(const std::string& resource)
		{
			std::string exe_path = executable_path();
#ifdef __APPLE__
			return exe_path + "/Resources/" + resource;
#else
			return exe_path + "/" + resource;
#endif
		}

		// -----------------------------------------------------------------------------------------------------------------------------------

#ifdef WIN32
		std::string executable_path()
		{
			if (g_exe_path == "")
			{
				char buffer[1024];
				GetModuleFileName(NULL, &buffer[0], 1024);
				g_exe_path = buffer;
				g_exe_path = path_without_file(g_exe_path);
			}

			return g_exe_path;
		}
#else
		std::string executable_path()
		{
			if (g_exe_path == "")
			{

			}

			return g_exe_path;
		}
#endif

		// -----------------------------------------------------------------------------------------------------------------------------------

		std::string current_working_directory()
		{
			char buffer[FILENAME_MAX];

			if (!GetCurrentDir(buffer, sizeof(buffer)))
				return "";

			buffer[sizeof(buffer) - 1] = '\0';
			return std::string(buffer);
		}

		// -----------------------------------------------------------------------------------------------------------------------------------

		void change_current_working_directory(std::string path)
		{
			ChangeWorkingDir(path.c_str());
		}

		// -----------------------------------------------------------------------------------------------------------------------------------

		std::string path_without_file(std::string filepath)
		{
#ifdef WIN32
			std::replace(filepath.begin(), filepath.end(), '\\', '/');
#endif
			std::size_t found = filepath.find_last_of("/\\");
			std::string path = filepath.substr(0, found);
			return path;
		}

		bool read_text(std::string path, std::string& out)
		{
			std::ifstream file;
			file.open(path);

			if (file.is_open())
			{
				file.seekg(0, std::ios::end);
				out.reserve(file.tellg());
				file.seekg(0, std::ios::beg);
				out.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

				return true;
			}
			else
				return false;
		}
	} // namespace utility
} // namespace dw