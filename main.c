#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE

#include <errno.h>
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
    snprintf(msg->data, width, "\e[%d;0H%s", line, message);
    msg->data[strlen(msg->data)] = ' ';
    *back_char(msg) = '\0';
    printf("%s", msg->data);
    fflush(stdout);
    free_char(msg);
}

vector_char* concat_path(const char* first, const char* second) {
    vector_char* result = reserve_junk_char(strlen(first) + 1 + strlen(second) + 1);
    sprintf(result->data, "%s/%s", first, second);
    return result;
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
    int saved_fd_for_copy = -1;
    vector_char* saved_full_path_for_copy = NULL;
    vector_char* saved_filename_for_copy = NULL;
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
        if (winsz.ws_row < 2) {
            do_error("I cannot work with this kind of terminal size, sorry :(", winsz.ws_row, winsz.ws_col);
            continue;
        }
        if (winsz.ws_row != prev.ws_row || winsz.ws_col != prev.ws_col || switched_folders) {
            need_to_redraw = true;
            cursor_line = 1;
            shift = 0;
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
                if (line->size > winsz.ws_col) {
                    line->data[winsz.ws_col - 1] = '\0';
                }
                printf("   %s", line->data);
                if (line->size + 3 - (13 * (current->data[i].type != DT_REG)) < winsz.ws_col) {
                    vector_char* spaces = create_char(winsz.ws_col - line->size - 3 + (13 * (current->data[i].type != DT_REG)), ' ');
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
            vector_char* name_of_file = concat_path(current_path, current->data[shift + cursor_line - 1].name);
            errno = 0;
            int ret = remove(name_of_file->data);
            if (ret == 0) {
                switched_folders = true;
                moved_cursor = true;
                if (cursor_line >= current->size) {
                    --cursor_line;
                }
            }
            else {
                vector_char* msg = create_char(winsz.ws_col, '\0');
                snprintf(msg->data, msg->size, "%s '%s' : %s", "Could not delete at path", name_of_file->data, strerror(errno));
                do_error(msg->data, winsz.ws_row, winsz.ws_col);
                free_char(msg);
            }
            free_char(name_of_file);
        }
        if (c == '\n') {
            do_error(current->data[shift + cursor_line - 1].name, winsz.ws_row, winsz.ws_col); // TODO : delete it
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
        if (c == 'c') {
            if (current->data[shift + cursor_line - 1].type == DT_DIR) {
                do_error("Directory copying is not supported, sorry :(", winsz.ws_row, winsz.ws_col);
                continue;
            }
            if (saved_fd_for_copy != -1) {
                free_char(saved_filename_for_copy);
                free_char(saved_full_path_for_copy);
                close(saved_fd_for_copy);
            }
            vector_char* name_of_file = concat_path(current_path, current->data[shift + cursor_line - 1].name);
            errno = 0;
            saved_fd_for_copy = open(name_of_file->data, O_RDONLY | O_LARGEFILE);

            if (saved_fd_for_copy < 0) {
                saved_fd_for_copy = -1;
                vector_char* msg = create_char(winsz.ws_col, '\0');
                snprintf(msg->data, msg->size, "%s '%s' : %s", "Failed to initiize copying of the file at ", name_of_file->data, strerror(errno));
                do_error(msg->data, winsz.ws_row, winsz.ws_col);
                free_char(msg);
                free_char(name_of_file);
                continue;
            }
            saved_full_path_for_copy = create_char(name_of_file->size, '\0');
            memcpy(saved_full_path_for_copy->data, name_of_file->data, name_of_file->size);

            saved_filename_for_copy = create_char(sizeof(current->data[shift + cursor_line - 1].name), '\0');
            memcpy(saved_filename_for_copy->data, current->data[shift + cursor_line - 1].name, saved_filename_for_copy->size);
            free_char(name_of_file);
        }
        if (c == 'v') {
            if (saved_fd_for_copy == -1) {
                do_error("Failed to insert a file : No file currently copied", winsz.ws_row, winsz.ws_col);
                continue;
            }
            vector_char* name_of_file = concat_path(current_path, saved_filename_for_copy->data);
            errno = 0;
            int fd_to_write = open(name_of_file->data, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE);

            if (fd_to_write < 0) {
                vector_char* msg = create_char(winsz.ws_col, '\0');
                snprintf(msg->data, msg->size, "%s '%s' : %s", "Failed to write a file to ", name_of_file->data, strerror(errno));
                do_error(msg->data, winsz.ws_row, winsz.ws_col);
                free_char(msg);
                free_char(name_of_file);
                continue;
            }

            static char buffer[4096];

            int to_write;
            while ((to_write = read(saved_fd_for_copy, buffer, 4096)) != 0) {
                int written = 0;
                do {
                    written += write(fd_to_write, buffer + written, to_write - written);
                } while (written != to_write);
            }

            close(fd_to_write);

            free_char(name_of_file);
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
                    if (shift + winsz.ws_row - 1 <= current->size - 1) {
                        ++shift;
                        force_redraw = true;
                    }
                    --cursor_line;
                }
                moved_cursor = true;
            }
        }
    }

    free_char(saved_full_path_for_copy);
    free_char(saved_filename_for_copy);
    tcsetattr(fd, TCSANOW, &old);
    free_DirectoryEntry(current);
    free(current_path);
    return 0;
}
