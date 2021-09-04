

#include <app.h>
#include <debug.h>
#include <stdio.h>


#include "testfunc.h"

extern void checkFoobinate(void);

/*******************************************************************************
 * Variables
 ******************************************************************************/
STATIC_COMMAND_START
STATIC_COMMAND("testfunc", "testfunc commands", &cmd_testfunc)
STATIC_COMMAND_END(testfunc);

int cmd_testfunc(int argc, const cmd_args *argv)
{
    printf("\nThis is the testfunc!\n");
    printf("We hope you like it!\n\n");

    checkFoobinate();

    return 0;
}

APP_START(testfunc)
    .flags = 0,
APP_END
