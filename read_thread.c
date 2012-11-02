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

#define _GNU_SOURCE

pthread_mutex_t mutex;

l_list *p3_data;

void *read_trd_fn(void *trd_data)
{
    // A new thread has been created, only one of its kind.
    // It was created in p2_actions
    // trd_data points to a struct r_trd_data which contains
    // (1) a linked list of all the lines from the input file
    // (2) a struct foo iterator of all the -m options supplied by the command line.
    struct r_trd_dat *infile_m_lines = (struct r_trd_dat *)trd_data;

    printf("Initting semaphore variable\n");
/*    sem_init(&search_sem, 0, 1); // Initial value of search_sem set to 1
    sem_init(&n_sem, 0, 0);      // n_sem for producer/consumer, init to 0
    sem_init(&more_data_sem, 0, 0);

    sem_init(&empty, 0, 1);
    sem_init(&full, 0, 0);*/
    pthread_mutex_init(&mutex, NULL);

    struct foo *m = infile_m_lines->m_list;
    l_list *infile_list = infile_m_lines->infile_list;

    // Create list of search threads
    s_list *search_threads = s_list_init();

    // initialize p3_data:
    p3_data = l_list_init();

    int s_count = 0;

    for(; m != NULL; m = m->next)
        s_count++;


    int i = 0;
    // create each search thread:
    for(m = infile_m_lines->m_list; m != NULL; m = m->next)
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

        pthread_join(s->tid, NULL);

        i++;
    }

    printf("s_count = %d\n", s_count);

    // Now all the search threads have been created (for 3 -m options supplied, there should be 3 threads)
    // Now put each line from the input file into each (search_thread's) s_node's _protected char*.

    node *end;
    for(end = p3_data->head; end != NULL; end = end->next)
        fprintf(infile_m_lines->fp, "%s", end->data);

    pthread_exit(NULL);
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
        //printf("Hi from search_thread_fn! My data is: %s, my thread id is: %u, protected is %s", s->_m,
             //(unsigned int)s->tid, iter->data);
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
            // Post data to collector thread.
            pthread_mutex_lock(&mutex);
            l_list_append(p3_data, iter->data);
            pthread_mutex_unlock(&mutex);
        }
    }

    pthread_exit(NULL);
    return NULL;
}

void *collector_trd_fn(void *col_data)
{
    // sem_wait on semaphore that will be posted when all data is ready to be passed to P3.
    node *p;
    char *holder;

    while(1)
    {
        //sem_wait(&full);
        pthread_mutex_lock(&mutex);

        holder = l_list_pop(p3_data);
        printf("\tGot p3_data: %s", holder);
        p = p3_data->head;
        
        pthread_mutex_unlock(&mutex);
        //sem_post(&empty);
    }

/*
    sem_wait(&n_sem);
    printf("waiting on search_sem\n");
    sem_wait(&search_sem);

    p = p3_data->head;
    while(p != NULL)
    {
        holder = l_list_pop(p3_data);
        printf("\tGot p3 data: %s", holder);
        p = p3_data->head;
    }

    sem_post(&search_sem);
*/

    pthread_exit(NULL);
}
