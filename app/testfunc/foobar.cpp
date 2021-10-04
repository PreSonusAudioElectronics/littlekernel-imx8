
#include <stdio.h>
#include <stdarg.h>
#include <util/utils.h>

using namespace Pep::Util;

class Foobar
{
public:
	void foobinate()
	{
		LKPRINTF("This is the C++ foobinator\n");
		LKPRINTF("\n How about this?  Is this working?\n");
		LKPRINTF("The meaning is: %1.2f\n", _theMeaning);
	}

private:
	float _theMeaning = 42;
};

Foobar foob1;


extern "C"{

void checkFoobinate(void)
{
	foob1.foobinate();
}

}