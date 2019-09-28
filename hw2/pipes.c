//Akshar Amin
//ECEC353

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

int main (void){
	int pipeEnds1[2];		//pipe is created with an array of 2
	int pipeEnds2[2];
	int pipedEnds1 = pipe(pipeEnds1);
	int pipedEnds2 = pipe(pipeEnds2);
	pid_t pid;
	pid = fork();			//pricess if forked to create a cloned child
	if (pipedEnds1 ==  -1 || pipedEnds2 == -1 || pid < 0){		//error detection
		fprintf(stderr, "error creating pipe or fork unsuccessful");
		return EXIT_FAILURE;
	 }
	if (pid > 0) {		//parent process
		int count;
		int eof = 5;		//is changed to the number of words in txt file
		int child_ret;
		char parentbuff1[25];		//25 is the buffer size
		char parentbuff2[25];
		for(count = 0;count < eof;count++){		//word is generated line by line in parent process
			scanf("%s",parentbuff1);
			write(pipeEnds1[1],parentbuff1,strlen(parentbuff1)+1);	//write to pipe
			child_ret = read(pipeEnds2[0],parentbuff2,25);
			printf("%s \n",parentbuff2);
		}
		close(pipeEnds1[1]);		//close old connectinos from pipe to file descriptors to avoid error
		close(pipeEnds2[0]);
		wait(&child_ret);			//wait for child
		return child_ret;
	  } 
	else {		//child process
		int count;
		int ret;
		char childbuff1[25];
		char childbuff2[25];
		close(pipeEnds1[1]);	//close write end of first pipe. 
		while((ret=read(pipeEnds1[0],childbuff1,25))>0){	//generate word line by line 
			for(count = 0;count < strlen(childbuff1);count++){	
				childbuff2[count] = toupper(childbuff1[count]);
			}
		childbuff2[strlen(childbuff1)] = '\0';
		write(pipeEnds2[1],childbuff2,strlen(childbuff1)+1);	
		}
		close(pipeEnds1[1]);
		close(pipeEnds1[0]);	//close 2 ends of pipes to make process seamless
		close(pipeEnds2[1]);
	}
	return EXIT_SUCCESS;
	
}