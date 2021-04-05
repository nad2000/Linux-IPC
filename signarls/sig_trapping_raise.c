#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void signal_handler(int sig);

int main() { 
	int ret;

	ret = signal(SIGINT,signal_handler);
	if (ret == SIG_ERR) {
		printf("Error: unable to set signlal handler...\n");
		exit(0);
	}

	printf("Going to raise a signal...\n");

	ret = raise(SIGINT);
	if (ret != 0) {
		printf("Error: unable to raise SIGINT signal.\n");
		exit(0);
	}

	printf("Exiting...\n");
	exit(0);
}


static void signal_handler(int sig) {
	printf("process caught signal %d\n", sig);
}
