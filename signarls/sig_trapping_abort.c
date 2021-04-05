#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void ctrlC_signal_handler(int sig);
static void abort_signal_handler(int sig);
int main() { 
	// Register user defined functions ctrlC_signal_handler
	signal(SIGINT, ctrlC_signal_handler);
	signal(SIGABRT, abort_signal_handler);
	char ch;
	printf("Abort process (y/n)?\n");
	scanf("%c", &ch);
	if (ch == 'y') abort();
	for (;;) sleep(1000);
	exit(0);
}


static void ctrlC_signal_handler(int sig) {
	printf("Ctrl-C pressed\n");
	printf("Bye Bye\n");
	exit(0);
}


static void abort_signal_handler(int sig) {
	printf("process is aborted\n");
	printf("Bye Bye\n");
	exit(0);
}
