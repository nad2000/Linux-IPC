Signals
=======

Definition: A signal is a system message sent form one process to another, not usually used to transfer data but instead used to remotely command the partnered process.

Signals can be sent by OS form the Kernel space or by one application to another application.

Signal is associated with a handler that can be 1) default, 2) customized (by a **signal handling routine**), or 3) simple ignore;

Signal handler routine are executed at the highest priority, preempting the normal execution flow of the process;

In Linux there are approximately 30 different signals. The most important:

 - SIGINT - interrupt (^C);
 - SIGUSR1 and SIGUSR2 - user defined signals;
 - SIGKILL - sent to process from kernel when kill -9 is invoked on PID. NB! This signal cannot be caught by the process.
 - SIGABRT - raised by abort() by the process itself. Again, it cannot be blocked;
 - SIGTERM - raised when *kill* is invoked. It can be caught by the process to execute user defined action.
 - SIGSEGV - segmentation fault, raised by the kernel to the process when illegal memory is referenced.
 - SIGCHILD = whenever a child terminates, this signal is sent to the parent. Upon receiving this signal, the parent should execute *wait()* system call to read child status.
 - SIGHUP ("signal hang up") is a signal sent to a process when its controlling terminal is closed. (It was originally designed to notify the process of a serial line drop.) Daemon programs sometimes use SIGHUP as a signal to restart themselves, the most common reason for this being to re-read a configuration file that has been changed.

Three ways of generating Signals:
---

 1. Raising a signal form OS to a process (e.g. hitting ^C);
 2. Sending a signal from a process to itself using *raise()*;
 3. Sending a signal from one process to another using *kill()* within a userspace;

Signal handling:
---

 1. 
