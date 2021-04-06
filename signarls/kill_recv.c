#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void signal_handler(int signal);
int main(int argc, char *argv[]) {

	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);

	printf("PID: %d\n", getpid());
	printf("Hit ENTER to exit...\n");
	getchar();
	return 0;
}

static void signal_handler(int signal) {
	printf("Signal '%d' recieved\n", signal);
}
