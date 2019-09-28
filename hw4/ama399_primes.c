#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

void *is_prime(unsigned int num){
	int c;
	for ( c = 2 ; c <= num - 1 ; c++ ){
		if ( num%c == 0 )
			return 0;
	}
	return 1;
}

void *handler(void *sig){
	return NULL;
}

int main(int argc, char* argv[]){
	int num = 123;
	pthread_t thread;
	pthread_create(&thread, NULL, handler, &num);
	pthread_join(thread, NULL);

	return EXIT_SUCCESS;
}