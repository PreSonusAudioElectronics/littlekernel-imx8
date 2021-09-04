

#include <app.h>
#include <debug.h>

#include <lib/console.h>


/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static int cmd_testfunc(int argc, const cmd_args *argv);

/*******************************************************************************
 * Variables
 ******************************************************************************/
STATIC_COMMAND_START
STATIC_COMMAND("testfunc", "testfunc commands", &cmd_testfunc)
STATIC_COMMAND_END(testfunc);


static int cmd_testfunc(int argc, const cmd_args *argv)
{
    printlk(LK_INFO, "\nThis is the testfunc!\n");
    printlk(LK_INFO, "We hope you like it!\n\n");

    return 0;
}

APP_START(testfunc)
    .flags = 0,
APP_END