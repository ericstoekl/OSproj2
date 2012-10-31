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

sem_t search_sem;
sem_t n_sem;
sem_t more_data_sem;

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

    printf("Initting semaphore variable\n");
    sem_init(&search_sem, 0, 1); // Initial value of search_sem set to 1
    sem_init(&n_sem, 0, 0);      // n_sem for producer/consumer, init to 0
    sem_init(&more_data_sem, 0, 0);

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

    
    // Loop through infile_list, posting each line to each thread's _protected data section with a semaphore.
    node *in_iter = infile_list->head;     // Data from input file
    s_node *s_iter; // linked list of threads
    int i, s_count = 0;
    for(; in_iter != NULL; in_iter = in_iter->next)
    {

        for(s_iter = search_threads->head; s_iter != NULL; s_iter = s_iter->next)
        {
            sem_wait(&search_sem);
            s_iter->_protected = in_iter->data;
            sem_post(&search_sem);
            sem_post(&n_sem);
            s_count++;
        }
        printf("s_count = %d\n", s_count);
        for(i = 0; i < s_count; i++)
            sem_post(&more_data_sem);
        s_count = 0;
    }

    // wait for each thread to finish:
    for(s_iter = search_threads->head; s_iter != NULL; s_iter = s_iter->next)
    {
        pthread_join(s_iter->tid, NULL);
    }

    return NULL;
}

void *search_trd_fn(void *s_data)
{
    // New thread has been created, that will be pushed into the search_threads list
    // s_data is the -m string that we need to search for.

    s_node *s = (s_node *)s_data;
    
    int len = strlen(s->_m);

    while(1)
    {
        sem_wait(&n_sem);
        sem_wait(&search_sem);

/**************************************************************************/
        char *begin = s->_protected;
        char *end = strchr(begin, '\0');      // terminating null char
        char *match = NULL;
        int matches = 0;
        while (begin < end && (match = strstr(begin, s->_m)) != NULL)
        {
            printf("Found %s in line %s, tid: %u\n", s->_m, s->_protected, (unsigned int)s->tid);
            matches++;
            begin = match + len;
        }
        //p->lines += (matches > 0);
        //p->matches += matches;
/**************************************************************************/
//        printf("Hi from search_thread_fn! My data is: %s, my thread id is: %u, protected is %s", s->_m, 
//            (unsigned int)s->tid, s->_protected);

        sem_post(&search_sem);
        sem_wait(&more_data_sem);
    }

    return NULL;
}

void *collector_trd_fn(void *col_data)
{

    // sem_wait on semaphore that will be posted when all data is ready to be passed to P3.
}
