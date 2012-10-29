#include "read_thread.h"
#include "infile_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// This makes Solaris and Linux happy about waitpid(); not required on Mac OS X
#include <sys/wait.h>

#include <fcntl.h>                                /* for open() */
#include <string.h>

#include <pthread.h>
#include <semaphore.h>

void *read_trd_fn(void *trd_data)
{
    struct r_trd_dat *infile_m_lines = (struct r_trd_dat *)trd_data;

/*
    // verify that data got passed in correctly:
    struct foo *p = infile_m_lines->m_list;
    for(; p != NULL; p = p->next)
        printf("%s\n", p->string);

    node *np = infile_m_lines->infile_list->head;
    for(; np != NULL; np = np->next)
        printf("infile: %s", np->data);
*/

    struct foo *m_list = infile_m_lines->m_list;
    l_list *infile_list = infile_m_lines->infile_list;

    // create each search thread:
    for(; m_list != NULL; m_list = m_list->next)
    {
        
    }
}
