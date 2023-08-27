#include "stdout_handler.h"
#include <stdio.h>

int stdout_write(LogHandler *_self, const char *src, int size)
{
    (void)_self;
    (void)size;
    return printf(src);
}

LogHandler stdout_log_handler = {
    .write = stdout_write,
    .read = NULL,
};
