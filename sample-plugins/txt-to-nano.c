#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

void open_file(const char* filename) {
    pid_t pid = fork();
    if (pid == -1) {
        return;
    } else if (pid == 0) {
        // child process
        char *args[] = { "nano", (char*)filename, NULL };
        execvp(args[0], args);
    } else {
        int status;
        pid_t child_pid;
        do {
            child_pid = waitpid(pid, &status, 0);
        } while (child_pid == -1);
    }
}
