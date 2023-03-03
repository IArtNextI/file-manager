#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include "manager.h"

int main() {
    int input_fd = set_up_input();
    if (input_fd < 0) {
        return 1;
    }

    TerminalSettings new_settings, old_settings;

    if (!load_current_terminal_settings(input_fd, &new_settings)) {
        fprintf(stderr, "Failed to load terminal settings");
        return 1;
    }

    old_settings = new_settings;

    setup_terminal_settings(&new_settings);

    if (!push_teminal_settings(input_fd, &new_settings)) {
        fprintf(stderr, "Failed to push terminal settings to the system");
        return 1;
    }

    char* caller_directory;
    if (!load_working_directory(&caller_directory)) {
        fprintf(stderr, "Failed to load working directory");
        return 1;
    }

    int ret = run_manager(input_fd, caller_directory);

    free(caller_directory);
    push_teminal_settings(input_fd, &old_settings);
    close(input_fd);
    return ret;
}
