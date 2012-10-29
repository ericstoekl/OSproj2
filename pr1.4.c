/* CMPSC 473, Project 1, solution
 *
 * pr1.0.c = original pr1pipe.c
 * pr1.1.c = three processes, two pipes
 * pr1.2.c = P1 reads file, P2 passes, P3 writes to stdout
 * pr1.3.c = final version. cleaned up (this version)
 *
 */

//--------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// This makes Solaris and Linux happy about waitpid(); not required on Mac OS X
#include <sys/wait.h>

#include <fcntl.h>                                /* for open() */
#include <string.h>

#include <pthread.h>

#include "read_thread.h"
#include "infile_list.h"

//--------------------------------------------------------------------------------

// print message and quit

//void err_sys(char *msg);
void err_usage(char *prog);

void p1_actions(char *file_in, int fd_out);
void p2_actions(struct foo *list, int fd_in, int fd_out);
void p3_actions(int fd_in, char *file_out);
// read from fd_in, write to fd_out
// fd = file descriptor, opened by pipe()
// treat fd as if it had come from open()

#define BUFFER_SIZE 4096

//--------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
// printf(" 1: PID, PPID: %d %d\n", getpid(), getppid());	// make some noise

// for use with getopt(3)
    int ch;
    extern char *optarg;
    extern int optind;
    extern int optopt;
    extern int opterr;

// for error messages
    char *prog = argv[0];

// check the command line first; this simplifies error handling along the pipeline
    char *input_file = "/dev/null";               // modify with the -i option
    char *output_file = "/dev/null";              // modify with the -o option
    struct foo *m_list = NULL;                    // modify with the -m option

    while ((ch = getopt(argc, argv, ":i:o:m:")) != -1)
    {
        switch (ch)
        {
            case 'i':                             // save this for P1
                input_file = strdup(optarg);
                break;
            case 'm':                             // save this for P2
                if (strlen(optarg) == 0)
                    { fprintf(stderr, "%s: empty string not allowed\n", prog); exit(0); }
                struct foo *p = malloc(sizeof(struct foo));

                if (p == NULL) err_sys("malloc (main)");
                p->string = strdup(optarg);
                if (p->string == NULL) err_sys("strdup (main)");

                p->length = strlen(p->string);
                p->lines = 0;
                p->matches = 0;
                p->next = m_list;                 // push to front of list
                m_list = p;
                break;
            case 'o':                             // save this for P3
                output_file = strdup(optarg);
                break;
            case '?':
                fprintf(stderr, "%s: invalid option '%c'\n", prog, optopt);
                err_usage(prog);
                break;
            case ':':
                fprintf(stderr, "%s: invalid option '%c' (missing argument)\n", prog, optopt);
                err_usage(prog);
                break;
            default:
                err_usage(prog);
                break;
        }
    }

    if (m_list == NULL)
    {
        fprintf(stderr, "warning: no -m option was supplied\n");
    }

// first pipe, P1 to P2
    int fd1[2];                                   // pipe endpoints
    pid_t p2_pid;

    if (pipe(fd1) < 0)
        { err_sys("pipe error (P1)"); }

    if ((p2_pid = fork()) < 0)
    {
        err_sys("fork error (P1)");
    }
    else if (p2_pid > 0)                          // parent (P1)
    {
        close(fd1[0]);                            // read from fd1[0]
        p1_actions(input_file, fd1[1]);           // write to fd1[1]
        if (waitpid(p2_pid, NULL, 0) < 0)         // wait for child (P2)
            { err_sys("waitpid error (P1)"); }
    }
    else                                      // child (P2)
    {
        close(fd1[1]);                            // write to fd1[1]

// second pipe, P2 to P3
        int fd2[2];                               // pipe endpoints
        pid_t p3_pid;

        if (pipe(fd2) < 0)
            { err_sys("pipe error (P2)"); }

        if ((p3_pid = fork()) < 0)
        {
            err_sys("fork error (P2)");
        }
        else if (p3_pid > 0)                      // parent (P2)
        {
            close(fd2[0]);                        // read from fd2[0]
// read from fd1[0], write to fd2[1]
            p2_actions(m_list, fd1[0], fd2[1]);
            if (waitpid(p3_pid, NULL, 0) < 0)     // wait for child (P3)
                { err_sys("waitpid error (P2)"); }
        }
        else                                  // child (P3)
        {
            close(fd2[1]);                        // write to fd2[1]
            p3_actions(fd2[0], output_file);      // read from fd2[0]
        }
    }

// printf(" 2: PID, PPID: %d %d\n", getpid(), getppid());	// make some noise

    return 0;
}


//--------------------------------------------------------------------------------

// print message and quit

/*void err_sys(char *msg)
{
    printf("error: PID %d, %s\n", getpid(), msg);
    exit(0);
}
*/

void err_usage(char *prog)
{
    fprintf(stderr, "usage: %s [-i ifile] [-o ofile] [-m string]\n", prog);
    fprintf(stderr, "  -i ifile     input file name; default is /dev/null\n");
    fprintf(stderr, "  -o ofile     output file name; default is /dev/null\n");
    fprintf(stderr, "  -m string    search string; can be repeated\n");

    exit(0);
}


//--------------------------------------------------------------------------------

// write to fd_out

void p1_actions(char *file_in, int fd_out)
{
    int count = 0;
    char line[BUFFER_SIZE];

#if 1
// C Standard I/O version

    FILE *fp_in = fopen(file_in, "r");
    if (fp_in == NULL)
        { err_sys("fopen(r) error (P1)"); }

    FILE *fp_out = fdopen(fd_out, "w");       // use fp_out as if it had come from fopen()
    if (fp_out == NULL)
        { err_sys("fdopen(w) error (P1)"); }

// The following is so we don't need to call fflush(fp_out) after each fprintf().
// The default for a pipe-opened stream is full buffering, so we switch to line
//   buffering.
// But, we need to be careful not to exceed BUFFER_SIZE characters per output
//   line, including the newline and null terminator.
    static char buffer_out[BUFFER_SIZE];          // off the stack, always allocated
                                                  // set fp_out to line-buffering
    int ret = setvbuf(fp_out, buffer_out, _IOLBF, BUFFER_SIZE);
    if (ret != 0)
        { err_sys("setvbuf(out) error (P1)"); }

    while (fgets(line, BUFFER_SIZE, fp_in) != NULL)
    {                                             // line ends with newline-null
        fprintf(fp_out, "%s", line);
        count += strlen(line);
    }

// input error or end-of-file; for this stage, it's not an error

    fclose(fp_in);
    fflush(fp_out);
    fclose(fp_out);

#else
// Unix I/O version

    int fd_in = open(file_in, O_RDONLY);
    if (fd_in == -1)
        { err_sys("open(r) error (P1)"); }

    int n;

    while ((n = read(fd_in, line, BUFFER_SIZE)) > 0)
    {
        write(fd_out, line, n);
        count += n;
    }

// input error or end-of-file; for this stage, it's not an error

    close(fd_in);
    close(fd_out);
#endif

    printf("P1: file %s, bytes %d\n", file_in, count);
}


//--------------------------------------------------------------------------------

// read from fd_in, write to fd_out

void p2_actions(struct foo *list, int fd_in, int fd_out)
{
/* If you want to use read() and write(), this works:
 *
 * char line[BUFFER_SIZE];
 *
 * int n = read(fd_in, line, BUFFER_SIZE);
 * write(fd_out, line, n);
 */

// see p1_actions() for similar code, with comments
    FILE *fp_in = fdopen(fd_in, "r");
    if (fp_in == NULL)
        { err_sys("fdopen(r) error (P2)"); }

    FILE *fp_out = fdopen(fd_out, "w");
    if (fp_out == NULL)
        { err_sys("fdopen(w) error (P2)"); }

    static char buffer_in[BUFFER_SIZE];
    static char buffer_out[BUFFER_SIZE];
    int ret;

    // Create list of lines from the input file, will be passed to the read_thread.
    l_list *infile_lines = l_list_init();

    ret = setvbuf(fp_in, buffer_in, _IOLBF, BUFFER_SIZE);
    if (ret != 0)
        { err_sys("setvbuf(in) error (P2)"); }

    ret = setvbuf(fp_out, buffer_out, _IOLBF, BUFFER_SIZE);
    if (ret != 0)
        { err_sys("setvbuf(out) error (P2)"); }

    char line[BUFFER_SIZE];

    // put each line into list of input file lines
    while(fgets(line, BUFFER_SIZE, fp_in) != NULL)
    {
        l_list_append(infile_lines, line);
    }

    // package both lists (input file lines and m_lines) into a r_trd_dat struct:
    struct r_trd_dat *trd_data = (struct r_trd_dat *)malloc(sizeof(struct r_trd_dat));
    if(trd_data == NULL)
        err_sys("trd_data malloc failed (p2_actions)");
    trd_data->m_list = list;
    trd_data->infile_list = infile_lines;

    // Initialize read_thread:
    pthread_t read_thread;
    int trd_err;

    trd_err = pthread_create(&read_thread, NULL, read_trd_fn, (void *)trd_data);
    if(trd_err != 0)
    {
        err_sys("error creating thread (p2_actions)");
    }


/*    while (fgets(line, BUFFER_SIZE, fp_in) != NULL)
    {                                             // line ends with newline-null
// look for a match
        int matched = 0;
        for (struct foo *p = list; p != NULL; p = p->next)
        {
            char *begin = line;
            char *end = strchr(begin, '\0');      // terminating null char
            char *match = NULL;
            int matches = 0;
            while (begin < end && (match = strstr(begin, p->string)) != NULL)
            {
                matches++;
                matched++;
                begin = match + p->length;
            }
            p->lines += (matches > 0);
            p->matches += matches;
        }
// should P2 forward all lines, or only the ones that match?
#if 0
        fprintf(fp_out, "%s", line);              // all lines
#else
                                                  // only a matching line
        if (matched > 0)
            fprintf(fp_out, "%s", line);
#endif
    }
*/
// input error or end-of-file; for this stage, it's not an error

    fclose(fp_in);
    fflush(fp_out);
    fclose(fp_out);

    for (struct foo *p = list; p != NULL; p = p->next)
    {
        printf("P2: string %s, lines %d, matches %d\n", p->string, p->lines, p->matches);
    }
}


//--------------------------------------------------------------------------------

// read from fd_in

void p3_actions(int fd_in, char *file_out)
{
/* If you want to use read() and write(), this works:
 *
 * char line[BUFFER_SIZE];
 *
 * int n = read(fd_in, line, BUFFER_SIZE);
 * write(STDOUT_FILENO, line, n);
 */

// see p1_actions() for similar code, with comments
    FILE *fp_in = fdopen(fd_in, "r");
    if (fp_in == NULL)
        { err_sys("fdopen(r) error (P3)"); }

    static char buffer_in[BUFFER_SIZE];
    int ret = setvbuf(fp_in, buffer_in, _IOLBF, BUFFER_SIZE);
    if (ret != 0)
        { err_sys("setvbuf(in) error (P3)"); }

    FILE *fp_out = fopen(file_out, "w");
    if (fp_out == NULL)
        { err_sys("fopen(w) error (P3)"); }

    char line[BUFFER_SIZE];
    int lines = 0;

    while (fgets(line, BUFFER_SIZE, fp_in) != NULL)
    {                                             // line ends with newline-null
        fprintf(fp_out, "%s", line);
        lines++;
    }

// input error or end-of-file; for this stage, it's not an error

    fclose(fp_in);
    fflush(fp_out);
    fclose(fp_out);

    printf("P3: file %s, lines %d\n", file_out, lines);
}


//--------------------------------------------------------------------------------
