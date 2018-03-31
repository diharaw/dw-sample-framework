#include "utility.h"

#include <fstream>

#ifdef WIN32
#include <Windows.h>
#endif

namespace Utility
{
	static std::string g_exe_path = "";

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

	bool ReadText(std::string path, std::string& out)
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
}