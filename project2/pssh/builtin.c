#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "builtin.h"
#include "parse.h"

static char* builtin[] = {
    "exit", 
    "which",
    "fg",
    "bg",
    "kill",
    "jobs",
    NULL
};


int is_builtin (char* cmd)
{
    int i;

    for (i=0; builtin[i]; i++) {
        if (!strcmp (cmd, builtin[i]))
            return 1;
    }

    return 0;
}


void builtin_execute (Task T)
{
    char* PATH = (char *) malloc(sizeof(char*)*strlen(getenv("PATH")));
    

    if (!strcmp (T.cmd, "exit")) {
        exit (EXIT_SUCCESS);
    }
    else if (!strcmp(T.cmd, "which")){
        if (T.argv[1]==NULL){
            exit(EXIT_SUCCESS);
        }
        if (is_builtin(T.argv[1])){
           printf("%s: shell built-in command\n", T.argv[1]);
           exit(EXIT_SUCCESS);
        }
        strcpy(PATH,getenv("PATH"));
        char* p = strtok(PATH,":");
        while (p != NULL) {
            char* probe = (char *) malloc(sizeof(char*)*strlen(p));
            strcpy(probe,p);
            strcat(probe,"/");
            strcat(probe,T.argv[1]);
            printf("%s\n",probe);
            free(PATH);
            free(probe);
            exit(EXIT_SUCCESS);
            p = strtok(NULL,":");
        }

        exit(EXIT_SUCCESS);
    }
    else {
        printf ("pssh: builtin command: %s (not implemented!)\n", T.cmd);
    }
}
