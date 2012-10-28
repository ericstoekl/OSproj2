#ifndef _INFILE_LIST
#define _INFILE_LIST

typedef struct line_node 
{
    struct line_node *next;
    char *data;
} node;

typedef struct line_list
{
    struct line_node *head;
    struct line_node *tail;
} l_list;

l_list *l_list_init();

int l_list_append(l_list *L, char *data);

void err_sys(char *msg);

#endif
