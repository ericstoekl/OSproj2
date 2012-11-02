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

char *p3_data;

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

    int s_count = 0;

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

        // Copy input file buffer over to this s node:
        s->infile_list = l_list_init();
        node *copy_iter = infile_list->head;
        for(; copy_iter != NULL; copy_iter = copy_iter->next)
        {
            l_list_append(s->infile_list, copy_iter->data);
        }

        // Append the newly created s_node to the list.
        if(s_list_append(search_threads, s) != 0)
            exit(0);

        pthread_create(&s->tid, NULL, search_trd_fn, (void *)s);

        //printf("sanity check: new thread m value: %s\n", s->_m);
        //for(copy_iter = s->infile_list->head; copy_iter != NULL; copy_iter = copy_iter->next)
            //printf("sanity check: infile_list line: %s", copy_iter->data);

        s_count++;
    }

    printf("s_count = %d\n", s_count);

    // Now all the search threads have been created (for 3 -m options supplied, there should be 3 threads)
    // Now put each line from the input file into each (search_thread's) s_node's _protected char*.

    s_node *joiner = search_threads->head;
    for(; joiner != NULL; joiner = joiner->next)
    {
        pthread_join(joiner->tid, NULL);
    }
    return NULL;
}

void *search_trd_fn(void *s_data)
{
    // New thread has been created, that will be pushed into the search_threads list
    // s_data is the -m string that we need to search for.

    s_node *s = (s_node *)s_data;
    
    int len = strlen(s->_m);

    node *iter;

    for(iter = s->infile_list->head; iter != NULL; iter = iter->next)
    {
        //sem_wait(&n_sem);
        //sem_wait(&search_sem);

        //printf("Hi from search_thread_fn! My data is: %s, my thread id is: %u, protected is %s", s->_m,
             //(unsigned int)s->tid, s->_protected);
        printf("Hi from search_thread_fn! My data is: %s, my thread id is: %u, protected is %s", s->_m,
             (unsigned int)s->tid, iter->data);
/**************************************************************************/
        char *begin = iter->data;
        char *end = strchr(begin, '\0');      // terminating null char
        char *match = NULL;
        int matches = 0;
        int matched = 0;
        while (begin < end && (match = strstr(begin, s->_m)) != NULL)
        {
            printf("Found %s in line %s, tid: %u\n", s->_m, iter->data, (unsigned int)s->tid);
            matches++;
            matched++;
            begin = match + len;
        }
        s->lines += (matches > 0);
        s->matches += matches;
/**************************************************************************/
        if(matched > 0)
        {
            printf("Starting semaphore part!!!!!!!!!!!\n");
            // Post data to collector thread.
            sem_wait(&search_sem);
            p3_data = strdup(iter->data);
            sem_post(&search_sem);
            sem_post(&n_sem);
        }

        //sem_post(&search_sem);
        //sem_wait(&more_data_sem);
    }

    return NULL;
}

void *collector_trd_fn(void *col_data)
{
    // sem_wait on semaphore that will be posted when all data is ready to be passed to P3.

    printf("Waiting on n_sem\n");
    sem_wait(&n_sem);
    printf("Waiting on search_sem\n");
    sem_wait(&search_sem);
    printf("\tGot p3 data: %s", p3_data);
    //fflush(stdout);
    sem_post(&search_sem);

}
