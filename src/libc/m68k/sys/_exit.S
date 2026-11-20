/*
 * _exit(status): exit does not return.
 */
#include "SYS.h"

ENTRY(_exit)
        SYSTRAP(exit)
END(_exit)
