#include <signal.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


int main(int argc, char** argv){
	pid_t pid;
	void(*old_handler);
	int sig, ret, i;

	if (argc == 1){
		printf("Usage: ./signal [options] <pid>\n");
		printf("Options:\n");
		printf("-s <signal> 	Sends <signal> to <pid>\n");
		printf("-l 		Lists all signal numbers with their names\n");
	}
	else if(strcmp(argv[1], "-s")==0){
		if (argc == 4){
			if (atoi(argv[2]) == 0){
				kill(atoi(argv[3]),0);
				if (errno == 3){
					printf("PID %s does not exist\n", argv[3]);
				}
				else if (errno == 1){
					printf("PID %s exists, but we can't send it signals\n", argv[3]);
				}
				else{
					printf("PID %s existsand is able to receive signals\n", argv[3]);
				}
			}
			else{
				kill(atoi(argv[3]),atoi(argv[2]));
			}
		}
		else{
			printf("Signal and PID needed\n");
		}
	}
	else if(strcmp(argv[1], "-l")==0){
		for (i = 1; i < (sizeof(sys_siglist)); i++) {
			printf("%i: %s\n",i, sys_siglist[i]);
		}
	}
	else if(argc == 2){
		kill(atoi(argv[1]),SIGTERM);
	}
	else{
		printf("Not a proper option");
	}
}