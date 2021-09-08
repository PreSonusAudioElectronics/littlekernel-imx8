
#include <stdio.h>

class Foobar
{
public:
    void foobinate()
    {
        printf("This is the C++ foobinator\n");
        printf("The meaning is: %1.2f\n", _theMeaning);
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