#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include "builtin.h"
#include "parse.h"
#include "managejobs.h"
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>


/*******************************************
 * Set to 1 to view the command line parse *
 *******************************************/
#define DEBUG_PARSE 0
#define MAX_JOBS 100

pid_t shell_terminal;
Job jobs[MAX_JOBS];

void set_fg_pgid(pid_t pgid);
int addjob(Job* jobs, Parse* P, char* cmd_cpy);
int findjob(Job* jobs, pid_t pid);
void jobs_cmd(Job* jobs);
void fg(Job* jobs, Parse* P);
void bg(Job* jobs, Parse* P);
void kill_cmd(Job* jobs, Parse* P);


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
    char cwd[1026];
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

int addjob(Job* jobs, Parse* P, char* cmd_cpy){
    int i;
    for (i = 0; i < MAX_JOBS; i++){
        if (jobs[i].npids == 0){
            if (P->background){
                jobs[i].status = BG;
            } else {
                jobs[i].status = FG;
            }
            jobs[i].name = malloc(sizeof(cmd_cpy));
            strcpy(jobs[i].name, cmd_cpy);
            free(cmd_cpy);
            return i;
        }
    }
    return -1; 
}

int findjob(Job* jobs, pid_t pid){
    int i,j;
    for (i = 0; i < MAX_JOBS; i++){
        for (j=0; j < jobs[i].npids; j++){
            if (jobs[i].pids[j] == pid){
                return i;
            }
        }
    }
    return -1;
}


void set_fg_pgid(pid_t pgid){
    void (*old)(int);
    old = signal(SIGTTOU, SIG_IGN);
    tcsetpgrp(STDIN_FILENO,pgid);
    tcsetpgrp(STDOUT_FILENO,pgid);
    signal(SIGTTOU, old);
}

void sigchild_handler(int sig)
{
    pid_t child;
    int status,jobnum;
    char buffer[1024];
    while (( child = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0){
        jobnum = findjob(jobs,child);
        if (WIFEXITED (status)){
            jobs[jobnum].pids_left -= 1;
            if (jobs[jobnum].pids_left == 0){
                if (jobs[jobnum].status == BG){
                    kill(jobs[jobnum].pgid, SIGTTOU);
                    printf("\n[%i] + done   %s\n",jobnum,jobs[jobnum].name); 
                }
                free(jobs[jobnum].name);
                free(jobs[jobnum].pids);
                jobs[jobnum].npids = 0;
                jobs[jobnum].pids_left = 0;
                jobs[jobnum].status = TERM;
                set_fg_pgid(shell_terminal);
            }
        } else if (WIFSIGNALED (status)) {
            set_fg_pgid(shell_terminal);
            free(jobs[jobnum].name);
            free(jobs[jobnum].pids);
            jobs[jobnum].npids = 0;
            jobs[jobnum].pids_left = 0;
            jobs[jobnum].status = TERM;
        } else if (WIFSTOPPED (status)) {
            set_fg_pgid(shell_terminal);
            jobs[jobnum].status = STOPPED;
            sprintf(buffer,"[%i] + suspended    %s\n", jobnum, jobs[jobnum].name);
            pid_t fg_pgid;
            fg_pgid = tcgetpgrp (STDOUT_FILENO);
            set_fg_pgid(getpgrp());
            printf("%s", buffer);
            set_fg_pgid(fg_pgid);
        } else if (WIFCONTINUED (status)) {
            jobs[jobnum].status = FG;
        } 
    }
}

void jobs_cmd(Job* jobs)
{   
    int i;
    char buffer[1024];
    for (i = 0; i < MAX_JOBS; i++){
        if (jobs[i].status == STOPPED){
            sprintf(buffer,"[%i] + stopped  %s\n",i, jobs[i].name);
            id_t fg_pgid;
            fg_pgid = tcgetpgrp (STDOUT_FILENO);
            set_fg_pgid(getpgrp());
            printf("%s", buffer);
            set_fg_pgid(fg_pgid);
        } else if ((jobs[i].status ==  FG) || (jobs[i].status == BG)){
            sprintf(buffer,"[%i] + running  %s\n",i, jobs[i].name);

            pid_t fg_pgid;
            fg_pgid = tcgetpgrp (STDOUT_FILENO);
            set_fg_pgid(getpgrp());
            printf("%s", buffer);
            set_fg_pgid(fg_pgid);
        }
    }
}

void fg(Job* jobs, Parse* P)
{
    int jobnum;

    if (P->tasks[0].argv[1] == NULL){
        printf("%s\n","Usage: fg %<job number>");
    } else {
        jobnum = atoi(P->tasks[0].argv[1]+1);
        if ((jobnum == 0) && jobs[jobnum].status == TERM){
            printf("pssh: invalid job number: %s\n", P->tasks[0].argv[1]);
        } else if (jobs[jobnum].status == TERM) {
            printf("pssh: invalid job number: %s\n", P->tasks[0].argv[1]);
        } else {    
            if (jobs[jobnum].status == STOPPED){
                jobs[jobnum].status = FG;
                printf("[%i] + continued    %s\n\n", jobnum ,jobs[jobnum].name);
                set_fg_pgid(jobs[jobnum].pgid);
                kill(-jobs[jobnum].pgid,SIGCONT);       
            } else {
                jobs[jobnum].status = FG;
                printf("%s\n\n", jobs[jobnum].name);
                set_fg_pgid(jobs[jobnum].pgid);
            }   
        }
    }
}

void bg(Job* jobs, Parse* P)
{
    int jobnum;

    if (P->tasks[0].argv[1] == NULL){
        printf("%s\n","Usage: bg %<job number>");
    } else {
        jobnum = atoi(P->tasks[0].argv[1]+1);
        if ((jobnum == 0) && jobs[jobnum].status == TERM){
            printf("pssh: invalid job number: %s\n", P->tasks[0].argv[1]);
        } else if (jobs[jobnum].status == TERM) {
            printf("pssh: invalid job number: %s\n", P->tasks[0].argv[1]);
        } else {    
            if (jobs[jobnum].status == STOPPED){
                jobs[jobnum].status = FG;
                printf("\n");
                printf("[%i] + continued        %s\n\n", jobnum ,jobs[jobnum].name);
                kill(-jobs[jobnum].pgid,SIGCONT);       
            } else {
                jobs[jobnum].status = FG;
                printf("\n");
                printf("%s\n\n", jobs[jobnum].name);
            }   
        }
    }
}

void kill_cmd(Job* jobs, Parse* P)
{
    int i,jobnum,sig,pid;

    if (P->tasks[0].argv[1] == NULL){
        printf("%s\n","Usage: kill [-s <signal>] <pid> | %<job> ...");
    } else {
        if (strcmp(P->tasks[0].argv[1],"-s") == 0) {
            // -s option used
            sig = atoi(P->tasks[0].argv[2]);
            if (P->tasks[0].argv[3][0] == '%') {
                // Killing jobs
                i = 3;
                while(P->tasks[0].argv[i] != NULL){
                    jobnum = atoi(P->tasks[0].argv[i]+1);
                    if (jobs[jobnum].status == TERM){
                        printf("pssh: invalid job number %i\n",jobnum);
                    } else {
                        kill(-jobs[jobnum].pgid,sig);
                        printf("[%i] + done %s\n",jobnum, jobs[jobnum].name);
                    }
                    i++;
                }
            } else {
                // -s options and PIDS!
                i = 3;
                while(P->tasks[0].argv[i] != NULL){
                    pid = atoi(P->tasks[0].argv[i]);
                    if (kill(pid,0) == 0){
                        kill(pid,sig);
                    } else {
                        printf("pssh: invalid pid number %i\n",pid);
                    }
                    i++;
                }
            }
        } else {
            if (P->tasks[0].argv[1][0] == '%'){
                i = 1;
                while(P->tasks[0].argv[i] != NULL){
                    jobnum = atoi(P->tasks[0].argv[i]+1);
                    if (jobs[jobnum].status == TERM){
                        printf("pssh: invalid job number %i\n",jobnum);
                    } else {
                        kill(-jobs[jobnum].pgid, SIGINT);
                        printf("[%i] + done %s\n",jobnum, jobs[jobnum].name);
                    }
                    i++;
                }
            } else {
                i = 1;
                while(P->tasks[0].argv[i] != NULL){
                    pid = atoi(P->tasks[0].argv[i]);
                    if (kill(pid,0) == 0){
                        kill(pid,SIGINT);
                    } else {
                        printf("pssh: invalid pid number %i\n",pid);
                    }
                    i++;
                }
            }   
        }
    }
}

/* Called upon receiving a successful parse.
 * This function is responsible for cycling through the
 * tasks, and forking, executing, etc as necessary to get
 * the job done! */
void execute_tasks (Parse* P, int jobnum)
{
    unsigned int t;
    pid_t* pid;
    signal(SIGCHLD, sigchild_handler);
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);

    jobs[jobnum].pids = malloc(P->ntasks * sizeof(*pid));
    jobs[jobnum].npids = P->ntasks;
    jobs[jobnum].pids_left = P->ntasks;

    pid = jobs[jobnum].pids;

    for (t = 0; t < P->ntasks; t++) {
        int fd[2];
        if (pipe(fd) == -1) {
            exit(EXIT_FAILURE);
        }

        pid[t]  = vfork();
        setpgid(pid[t], pid[0]);
                
        if ((t == 0) && (pid[t] > 0)) {
            jobs[jobnum].pgid = pid[0];
            if (jobs[jobnum].status != BG){
                set_fg_pgid(jobs[jobnum].pgid);
            } 
            else {
                set_fg_pgid(shell_terminal);    
            }
        }
        if (pid < 0) {
            free(jobs[jobnum].name);
            free(jobs[jobnum].pids);
            jobs[jobnum].npids = 0;
            jobs[jobnum].pids_left = 0;
            jobs[jobnum].status = TERM;
            exit(EXIT_FAILURE);
        } 
        else if (pid[t] == 0) {
            if (t==0 && P->ntasks > 1){
                dup2(fd[1],0);
            }                                                                                    
         
            else if (t < (P->ntasks-1) && P->ntasks > 1) {
                dup2(fd[0],0);                             
                dup2(fd[1],1);                              
            }

            else if (t == (P->ntasks-1) && P->ntasks > 1) {
                dup2(fd[0],0);                              
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
    
        if (jobs[jobnum].status == BG){
            printf("[%i] ", jobnum);
            for (t = 0; t < jobs[jobnum].npids; t++) {
                printf("%i ", pid[t]);
            }   
            printf("\n");   
    
    }
}



int main (int argc, char** argv)
{
    char* cmdline;
    char* cmd_cpy;
    Parse* P;
    int jobnum;
    shell_terminal = getpgrp();

    print_banner ();

    while (1) {
        while(tcgetpgrp(STDIN_FILENO) != getpid()){
            pause();
        }

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        cmdline = readline (build_prompt());
        if (!cmdline)
            exit (EXIT_SUCCESS);
        cmd_cpy = malloc(sizeof(cmdline));
        strcpy(cmd_cpy,cmdline);
        
        if (strcmp(cmdline, "exit")== 0){
            free(cmdline);
            exit(EXIT_SUCCESS);
        }
        P = parse_cmdline (cmdline);
        if (!P)
            parse_destroy (&P);
            free(cmdline);

        if (P->invalid_syntax) {
            printf ("pssh: invalid syntax\n");
            parse_destroy (&P);
            free(cmdline);
            
        }
        int i;
        for (i = 0; i < P->ntasks; i++){
            if (!command_found(P->tasks[i].cmd) && !is_builtin(P->tasks[i].cmd)){
                parse_destroy (&P);
                free(cmdline);
                
            }
        }
        if (strcmp(P->tasks[0].cmd, "jobs")==0){
            jobs_cmd(jobs);
            parse_destroy (&P);
            free(cmdline);
            
        } 
        else if (strcmp(P->tasks[0].cmd, "fg")==0){
            fg(jobs,P);
            parse_destroy (&P);
            free(cmdline);
            
        } 
        else if (strcmp(P->tasks[0].cmd, "bg")==0){
            bg(jobs,P);
            parse_destroy (&P);
            free(cmdline);
            
        } 
        else if (strcmp(P->tasks[0].cmd, "kill")==0){
            kill_cmd(jobs,P);
            parse_destroy (&P);
            free(cmdline);
            
        }

        jobnum = addjob(jobs, P, cmd_cpy);
        if (jobnum == -1){
            printf("Failed to create job\n");
            parse_destroy (&P);
            free(cmdline);
            
        }
        execute_tasks (P, jobnum);

    }
}
