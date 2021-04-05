#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void ctrlC_signal_handler(int sig);
int main() { 
	// Register user defined functions ctrlC_signal_handler
	signal(SIGINT, ctrlC_signal_handler);
	for (;;) sleep(1000);
}


static void ctrlC_signal_handler(int sig) {
	printf("Ctrl-C pressed\n");
	printf("Bye Bye\n");
	exit(0);
}
