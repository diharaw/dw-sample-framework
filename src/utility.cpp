#include "utility.h"

#include <fstream>

namespace Utility
{
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





