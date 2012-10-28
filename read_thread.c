#include "read_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// This makes Solaris and Linux happy about waitpid(); not required on Mac OS X
#include <sys/wait.h>

#include <fcntl.h>                                /* for open() */
#include <string.h>

#include <pthread.h>

void *read_thread(int fd_read)
{
}
