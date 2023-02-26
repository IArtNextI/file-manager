#define _DEFAULT_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <termios.h>

#include "terminal.h"

bool read_char_fd(int fd, char* c) {
    int ret = read(fd, c, 1);
    if (ret == -1) {
        fprintf(stderr, "Sonmething went wrong");
        exit(1);
    }
    if (ret == 0) return false;
    return true;
}

bool is_one_dot(const char* str) {
    return str[0] == '.' && str[1] == '\0';
}

bool is_two_dots(const char* str) {
    return str[0] == '.' && str[1] == '.' && str[2] == '\0';
}

void clear_error_line(int line, int width) {
    vector_char* spaces = create_char(width + 1, ' ');
    *back_char(spaces) = '\0';
    printf("\e[%d0H%s", line, spaces->data);
    free_char(spaces);
}

void do_error(const char* message, int line, int width) {
    vector_char* msg = create_char(width, ' ');
    snprintf(msg->data, width - 1, "\e[%d;0H%s", line, message);
    msg->data[strlen(msg->data)] = ' ';
    *back_char(msg) = '\0';
    printf(msg->data);
    fflush(stdout);
    free_char(msg);
}

int main() {
    int fd = open("/dev/tty", O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Failed to open /dev/tty");
        return 1;
    }

    // Save terminal settings
    struct termios terms, old;
    tcgetattr(fd, &terms);

    old = terms;

    terms.c_lflag &= ~(ICANON | ECHO);
    terms.c_cc[VMIN] = 1;
    terms.c_cc[VTIME] = 0;

    // update terminal settings
    tcsetattr(fd, TCSANOW, &terms);

    // get terminal size

    // fetch current path
    char* current_path = getcwd(NULL, 0);
    if (current_path == NULL) {
        fprintf(stderr, "Insufficient memory");
        return 1;
    }
    // Load current directory

    int cursor_line = 1;

    struct winsize prev;
    if (!get_terminal_size(&prev)) {
        fprintf(stderr, "Failed to fetch terminal size");
        return 1;
    }
    bool switched_folders = true;
    bool force_redraw = true;
    bool moved_cursor = true;
    vector_DirectoryEntry* current = NULL;
    int shift = 0;
    while (true) {
        bool need_to_redraw = false;
        if (switched_folders) {
            shift = 0;
            need_to_redraw = true;
            switched_folders = false;
        }
        if (force_redraw) {
            need_to_redraw = true;
        }
        force_redraw = false;
        struct winsize winsz;
        if (!get_terminal_size(&winsz)) {
            fprintf(stderr, "Failed to fetch terminal size");
            return 1;
        }
        if (winsz.ws_row != prev.ws_row || winsz.ws_col != prev.ws_col || switched_folders) {
            need_to_redraw = true;
        }
        prev = winsz;
        if (need_to_redraw) {
            if (current != NULL) {
                free_DirectoryEntry(current);
            }
            current = load_directory(current_path);
            sort_directory_entries(current);
            printf("\e[H");
            int to_print = winsz.ws_row - 1;
            if (to_print > current->size) {
                to_print = current->size;
            }
            for (int i = shift; i < shift + to_print; ++i) {
                vector_char* line = form_line(current_path, &current->data[i]);
                printf("   %s", line->data);
                if (line->size + 3 < winsz.ws_col) {
                    vector_char* spaces = create_char(winsz.ws_col - line->size - 3, ' ');
                    *(back_char(spaces)) = '\0';
                    printf("%s\n", spaces->data);
                    free_char(spaces);
                }
                else {
                    printf("\n");
                }

                free_char(line);
            }
            for (int i = current->size; i < winsz.ws_row - 1; ++i) {
                vector_char* spaces = create_char(winsz.ws_col - 2, ' ');
                *(back_char(spaces)) = '\0';
                printf("%s\n", spaces->data);
                free_char(spaces);
            }
        }
        if (moved_cursor) {
            printf("\033[?25l");
            printf("\e[%d;2H\b\b->", cursor_line);
            if (cursor_line > 1) {
                printf("\e[%d;2H\b\b  ", cursor_line - 1);
            }
            if (cursor_line < winsz.ws_row - 1) {
                printf("\e[%d;2H\b\b  ", cursor_line + 1);
            }
        }
        fflush(stdout);
        char c;
        if (!read_char_fd(fd, &c)) {
            break;
        }
        clear_error_line(winsz.ws_row, winsz.ws_col);
        if (c == 'q') {
            break;
        }
        moved_cursor = false;
        if (c == 'd') {
            if (is_one_dot(current->data[shift + cursor_line - 1].name) || is_two_dots(current->data[shift + cursor_line - 1].name)) {
                continue;
            }
            vector_char* name_of_file = reserve_junk_char(strlen(current_path) + 1 + strlen(current->data[shift + cursor_line - 1].name) + 1);
            sprintf(name_of_file->data, "%s/%s", current_path, current->data[shift + cursor_line - 1].name);
            int ret = remove(name_of_file->data);
            free_char(name_of_file);
            if (ret == 0) {
                switched_folders = true;
                moved_cursor = true;
                if (cursor_line >= current->size) {
                    --cursor_line;
                }
            }
            else {
                do_error("Something went wrong", winsz.ws_row, winsz.ws_col);
            }
        }
        if (c == '\n') {
            do_error(current->data[shift + cursor_line - 1].name, winsz.ws_row, winsz.ws_col);
            if (current->data[shift + cursor_line - 1].type == DT_DIR) {
                int len = strlen(current_path);
                if (is_one_dot(current->data[shift + cursor_line - 1].name)) {
                    continue;
                }
                if (is_two_dots(current->data[shift + cursor_line - 1].name)) {
                    int ptr = strlen(current_path) - 1;
                    while (current_path[ptr] != '/') {
                        current_path[ptr] = '\0';
                        ptr--;
                    }
                    current_path[ptr] = '\0';
                    current_path[0] = '/';
                }
                else {
                    char* other = calloc(len + 1 + strlen(current->data[shift + cursor_line - 1].name) + 1, 1);
                    sprintf(other, "%s/%s", current_path, current->data[shift + cursor_line - 1].name);
                    free(current_path);
                    current_path = other;
                }
                switched_folders = true;
                moved_cursor = true;
                cursor_line = 1;
            }
        }
        if (c == '\e') {
            if (!read_char_fd(fd, &c)) { // [ symbol is useless
                break;
            }
            if (!read_char_fd(fd, &c)) {
                break;
            }
            if (c == 'A') {
                --cursor_line;
                if (cursor_line == 0) {
                    if (shift > 0) {
                        --shift;
                        force_redraw = true;
                    }
                    ++cursor_line;
                }
                moved_cursor = true;
            } else if (c == 'B') {
                ++cursor_line;
                if (cursor_line == winsz.ws_row || cursor_line == current->size + 1) {
                    if (shift + winsz.ws_row - 1 < current->size - 1) {
                        ++shift;
                        force_redraw = true;
                    }
                    --cursor_line;
                }
                moved_cursor = true;
            }
        }
    }

    tcsetattr(fd, TCSANOW, &old);
    free_DirectoryEntry(current);
    free(current_path);
    return 0;
}