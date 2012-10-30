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
    // A new thread has been created, only one of its kind.
    // It was created in p2_actions
    // trd_data points to a struct r_trd_data which contains
    // (1) a linked list of all the lines from the input file
    // (2) a struct foo iterator of all the -m options supplied by the command line.
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
        s->_m = strdup(m->string);
        if(s->_m == NULL)
            err_sys("strdup (read_trd_fn)");

        pthread_create(&s->tid, NULL, search_trd_fn, (void *)s);

        printf("sanity check: new thread m value: %s\n", s->_m);

        // Append the newly created s_node to the list.
        if(s_list_append(search_threads, s) != 0)
            exit(0);
    }

    // Now all the search threads have been created (for 3 -m options supplied, there should be 3 threads)
    // Now put each line from the input file into each (search_thread's) s_node's _protected char*.
}

void *search_trd_fn(void *s_data)
{
    // New thread has been created, that will be pushed into the search_threads list
    // s_data is the -m string that we need to search for.

    s_node *s = (s_node *)s_data;

    printf("Hi from search_thread_fn! My data is: %s, my thread id is: %u or %u\n", s->_m, 
        (unsigned int)s->tid, (unsigned int)pthread_self());

}




