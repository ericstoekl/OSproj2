#include "infile_list.h"

#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>


/*
typedef struct line_node 
{
    struct line_node *next;
    char *line;
} node;

typedef struct line_list
{
    struct line_node *head;
    struct line_node *tail;
} l_list;
*/

l_list *l_list_init()
{
    l_list *L = malloc(sizeof(l_list));
    L->head = L->tail = NULL;

    return L;
}

int l_list_append(l_list *L, char *data)
{
    node *p = malloc(sizeof(node));
    if(p == NULL)
    {
        err_sys("malloc (l_list_append)");
        return -1;
    }

    p->next = NULL;
    p->data = strdup(data);
    if(p->data == NULL)
    {
        err_sys("strdup (l_list_append)");
        return -1;
    }

    if(L->head == NULL)
    {
        L->head = L->tail = p;
    }
    else
    {
        L->tail->next = p;
        L->tail = p;
    }

    return 0;
}

void err_sys(char *msg)
{
    printf("error: PID %d, %s\n", getpid(), msg);
    exit(0);
}
