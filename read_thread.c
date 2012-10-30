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

    struct foo *m = infile_m_lines->m_list;
    l_list *infile_list = infile_m_lines->infile_list;

    // Create list of search threads
    s_list *search_threads = s_list_init();

    // create each search thread:
    for(; m != NULL; m = m->next)
    {
        s_node *s = (s_node *)malloc(sizeof(s_node));
        if(s == NULL)
            err_sys("malloc (read_trd_fn)");
    
        s->next = NULL;
        pthread_create(s->tid, NULL, search_trd_fn, (void *)m->string);
        s->_m = strdup(m->string);
        if(s->_m == NULL)
            err_sys("strdup (read_trd_fn)");

        printf("sanity check: new thread m value: %s\n", s->_m);

        // Append the newly created s_node to the list.
        if(s_list_append(search_threads, s) != 0)
            exit(0);
    }
}

void *search_trd_fn(void *s_data)
{

}




