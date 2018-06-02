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