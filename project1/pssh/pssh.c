#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>

#include "builtin.h"
#include "parse.h"

/*******************************************
 * Set to 1 to view the command line parse *
 *******************************************/
#define DEBUG_PARSE 0



void print_banner ()
{
    printf ("                    ________   \n");
    printf ("_________________________  /_  \n");
    printf ("___  __ \\_  ___/_  ___/_  __ \\ \n");
    printf ("__  /_/ /(__  )_(__  )_  / / / \n");
    printf ("_  .___//____/ /____/ /_/ /_/  \n");
    printf ("/_/ Type 'exit' or ctrl+c to quit\n\n");
}


/* returns a string for building the prompt
 *
 * Note:
 *   If you modify this function to return a string on the heap,
 *   be sure to free() it later when appropirate!  */
static char* build_prompt ()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL){
        fprintf(stdout, "%s", cwd);
    }
    return  "$ ";
}


/* return true if command is found, either:
 *   - a valid fully qualified path was supplied to an existing file
 *   - the executable file was found in the system's PATH
 * false is returned otherwise */
static int command_found (const char* cmd)
{
    char* dir;
    char* tmp;
    char* PATH;
    char* state;
    char probe[PATH_MAX];

    int ret = 0;

    if (access (cmd, X_OK) == 0)
        return 1;

    PATH = strdup (getenv("PATH"));

    for (tmp=PATH; ; tmp=NULL) {
        dir = strtok_r (tmp, ":", &state);
        if (!dir)
            break;

        strncpy (probe, dir, PATH_MAX);
        strncat (probe, "/", PATH_MAX);
        strncat (probe, cmd, PATH_MAX);

        if (access (probe, X_OK) == 0) {
            ret = 1;
            break;
        }
    }

    free (PATH);
    return ret;
}


/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks (Parse* P)
{
    unsigned int t;
    pid_t pid;
    pid = vfork();
    for (t = 0; t < P->ntasks; t++) {
        int fd[2];
        if (pid < 0) {
        } 
        else if (pid == 0) {
            if (P->ntasks > 1 && (t+1)!=P->ntasks) {
                dup2(fd[1],0);                              //fd[1] = Write side of the pipe
                                                            //dup2(x,0), the 0 is the file descriptor of STDIN
            } 
            else if (t < (P->ntasks-1) && P->ntasks > 1) {
                dup2(fd[0],0);                              //connect to previous pipe
                dup2(fd[1],1);                              //dup2(x,1), the 1 is the file descriptor of STDOUT
            }

            else if (t == (P->ntasks-1) && P->ntasks > 1) {
                dup2(fd[0],0);                              //conect to previous pipe
            }
            if (is_builtin (P->tasks[t].cmd)) {
               if (P->outfile && t == (P->ntasks-1)){
                    FILE *out;
                    out = fopen(P->outfile,"w");
                    dup2(fileno(out),1);
                }
                if (P->infile){
                    FILE *in;
                    in = fopen(P->infile,"r");
                    dup2(fileno(in),0);
                }
                builtin_execute (P->tasks[t]);
            } 
            else if (command_found (P->tasks[t].cmd)) {
                if (P->outfile && t == (P->ntasks-1)){
                    FILE *out;
                    out = fopen(P->outfile,"w");
                    dup2(fileno(out),1);
                }
                if (P->infile){
                    FILE *in;
                    in = fopen(P->infile,"r");
                    dup2(fileno(in),0);
                }
                execvp(P->tasks[t].cmd,&P->tasks[t].argv[0]);
                printf ("pssh: found but can't exec: %s\n", P->tasks[t].cmd);
            } 
            else {
                printf ("pssh: command not found: %s\n", P->tasks[t].cmd);
                break;
            }
        }
        else {
            if (strcmp(P->tasks[t].cmd, "exit") == 0){
                builtin_execute (P->tasks[t]);
            }
            if (0 != 0){
                close(0);
            }
            close(fd[1]);
            if (t == (P->ntasks-1)){
                close(fd[0]);
            }
        } 

    }
    for (t = 0; t < P->ntasks; t++) {
        wait(NULL);
    }

}


int main (int argc, char** argv)
{
    char* cmdline;
    Parse* P;

    print_banner ();

    while (1) {
        cmdline = readline (build_prompt());
        if (!cmdline)       /* EOF (ex: ctrl-d) */
            exit (EXIT_SUCCESS);

        P = parse_cmdline (cmdline);
        if (!P)
            goto next;

        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
            goto next;
        }

#if DEBUG_PARSE
        parse_debug (P);
#endif

        execute_tasks (P);

    next:
        parse_destroy (&P);
        free(cmdline);
    }
}
