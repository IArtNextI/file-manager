#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "terminal.h"

bool read_char_fd(int fd, char* c);

bool is_one_dot(const char* str);

bool is_two_dots(const char* str);

void clear_error_line(int line, int width);

void do_error(const char* message, int line, int width);

vector_char* concat_path(const char* first, const char* second);

int set_up_input();

typedef struct termios TerminalSettings;

bool load_current_terminal_settings(int input_fd, TerminalSettings* settings);

void setup_terminal_settings(TerminalSettings* settings);

bool push_teminal_settings(int input_fd, TerminalSettings* settings);

bool load_working_directory(char** line);

int run_manager(int input_fd, const char* caller_directory);
