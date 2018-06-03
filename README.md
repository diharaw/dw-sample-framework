# dwSampleFramework

## What is it?
A simple C++ framework for implementing graphics technique samples.

## Features
* Cross-platform
* Mesh loading
* Texture loading
* Input
* Configuration via JSON
* Debug drawing
* Graphics API abstraction via Terminus-GFX

## Sample
Creating a project using dwSampleFramework is as easy as inheriting from the dw::Application class and overriding the methods you need.

```c++
#include <application.h>

class Sample : public dw::Application
{
protected:
	bool init(int argc, const char* argv[]) override
	{
		return true;
	}

	void update(double delta) override
	{

	}

	void shutdown() override
	{

	}
};

DW_DECLARE_MAIN(Sample)
``` 
A more detailed sample can be found in the sample directory.

## Dependencies

* [Terminus-GFX](https://github.com/diharaw/Terminus-GFX) 
* [GLFW](https://github.com/glfw/glfw) 
* [Assimp](https://github.com/assimp/assimp) 
* [json](https://github.com/nlohmann/json) 
* [glm](https://github.com/g-truc/glm) 
* [imgui](https://github.com/ocornut/imgui) 
* [stb](https://github.com/nothings/stb) 

## License

```
Copyright (c) 2018 Dihara Wijetunga

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT 
LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```