#include <sys/types.h> /* pid_t                              */
#include <sys/wait.h>  /* waitpid(), WEXITSTATUS()           */
#include <unistd.h>    /* vfork()                            */
#include <stdlib.h>    /* exit(), EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>     /* printf(), fprintf(), stderr        */

int main(int argc, char** argv)
{
    pid_t pid;

    pid = vfork();

    if (pid < 0) {
        fprintf(stderr, "error -- failed to vfork()");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* only executed by the PARENT process */
        int child_ret;

        printf("parent -- pid: %i\n", getpid());
        waitpid(pid, &child_ret, 0);
        printf("parent -- child exited with code %i\n", WEXITSTATUS(child_ret));

        exit(EXIT_SUCCESS);
    } else {
        /* only executed by the CHILD process */

        printf("child -- pid: %i\n", getpid());
        execlp("/bin/ls", "ls","-lh", NULL);

        /* This will not execute since it no longer exists in the process
         * address space of the child after the exec*() call successfully
         * rewrites the address space with the contents of the hello_world
         * program. */
        printf("child -- error: failed to exec '1-hello_world'\n");
        exit(EXIT_FAILURE);
    }
}
