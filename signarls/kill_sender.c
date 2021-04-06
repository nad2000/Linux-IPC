#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {

	if (argc < 1) {
		printf("Need a PID and optionally 1 or 2 to specify the signal...\n");
		exit(-1);
	}

	int signo;
	pid_t pid;
	pid = (pid_t)atoi(argv[1]);
	signo = argc > 2 && argv[2][0] == '2' ? SIGUSR2 : SIGUSR1;
	kill(pid, signo);
	printf("Hit ENTER to exit...\n");
	getchar();
	return 0;
}
