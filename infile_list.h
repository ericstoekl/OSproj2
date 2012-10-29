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

// list of strings
struct foo
{ 
    struct foo *next;
    char *string;
    int length;
    int lines;
    int matches;
};

// Struct for data passed in read_thread
struct r_trd_dat
{
    struct foo *m_list;
    l_list *infile_list;
};

// struct for data passed into each search_thread
struct s_trd_dat
{
    char *_m;           // search term from command line -m option
    char *_protected;   // line from input file to compare _m to, critical section of memory.
                        // use semaphore to make sure data has written to this
                        // before you try to read from it.
};

l_list *l_list_init();

int l_list_append(l_list *L, char *data);

void err_sys(char *msg);

#endif
