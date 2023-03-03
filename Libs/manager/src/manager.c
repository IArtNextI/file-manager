#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE

#include "manager.h"
#include "terminal.h"
#include <dlfcn.h>

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
    vector_char* result = create_char(strlen(first) + 1 + strlen(second) + 1, '\0');
    sprintf(result->data, "%s/%s", first, second);
    return result;
}

int set_up_input() {
    int fd = open("/dev/tty", O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Failed to open /dev/tty");
        return -1;
    }
    return fd;
}

bool load_current_terminal_settings(int input_fd, TerminalSettings* settings) {
    return tcgetattr(input_fd, settings) == 0;
}

void setup_terminal_settings(TerminalSettings* settings) {
    settings->c_lflag &= ~(ICANON | ECHO);
    settings->c_cc[VMIN] = 1;
    settings->c_cc[VTIME] = 0;
}

bool push_teminal_settings(int input_fd, TerminalSettings* settings) {
    return tcsetattr(input_fd, TCSANOW, settings) == 0;
}

bool load_working_directory(char** line) {
    (*line) = getcwd(NULL, 0);
    return (*line) != NULL;
}

int run_manager(int input_fd, const char* caller_directory) {
    char* current_path;
    if (!load_working_directory(&current_path)) {
        fprintf(stderr, "Failed to load current working directory name");
        return 1;
    }

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
    mode_t saved_mode_for_copy = 0;
    bool need_to_delete_source_for_copy = false;
    bool filter_out_hidden_files = false;
    bool had_files_to_drop = false;
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
            if (!current) {
                do_error("Failed to load directory", winsz.ws_row, winsz.ws_col);
                continue;
            }
            if (filter_out_hidden_files) {
                had_files_to_drop = false;
                int insz = current->size;
                int ok_ptr = insz - 1;
                for (int i = 0; i < insz; ++i) {
                    if (is_one_dot(current->data[i].name) || is_two_dots(current->data[i].name)) {
                        continue;
                    }
                    if (current->data[i].name[0] == '.') {
                        if (ok_ptr > i) {
                            had_files_to_drop = true;
                            DirectoryEntry tmp;
                            tmp = current->data[ok_ptr];
                            current->data[ok_ptr] = current->data[i];
                            current->data[i] = tmp;
                            current->size--;
                            --i;
                            --ok_ptr;
                        }
                    }
                }
            }
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
        if (!read_char_fd(input_fd, &c)) {
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
                snprintf(msg->data, msg->size, "%s : %s", "Could not delete", strerror(errno));
                do_error(msg->data, winsz.ws_row, winsz.ws_col);
                free_char(msg);
            }
            free_char(name_of_file);
        }
        else if (c == '\n') {
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
                    errno = 0;
                    if (access(other, R_OK) != 0) {
                        vector_char* msg = create_char(winsz.ws_col, ' ');
                        snprintf(msg->data, msg->size, "%s : %s", "Failed to open directory", strerror(errno));
                        do_error(msg->data, winsz.ws_row, winsz.ws_col);
                        free_char(msg);
                        free(other);
                        continue;
                    }
                    free(current_path);
                    current_path = other;
                }
                switched_folders = true;
                moved_cursor = true;
                cursor_line = 1;
            }
            else if (current->data[shift + cursor_line - 1].type == DT_REG) {
                // else need to look up a dynamic lib
                int sz = strlen(current->data[shift + cursor_line - 1].name);
                int ptr = sz - 1;
                while (ptr >= 0 && current->data[shift + cursor_line - 1].name[ptr] != '.') {
                    --ptr;
                }
                if (ptr < 0) {
                    do_error("I don't know how to work with this file", winsz.ws_row, winsz.ws_col);
                    continue;
                }
                vector_char* extension = reserve_char(sz - ptr + 20, '\0');
                for (++ptr; ptr < sz; ++ptr) {
                    push_back_char(extension, current->data[shift + cursor_line - 1].name[ptr]);
                }
                vector_char* required_filename = create_char(strlen(caller_directory) + 1 + 3 + sz - ptr + 50, '\0');
                snprintf(required_filename->data, required_filename->size, "%s/file-manager-plugins/lib%s.so", caller_directory, extension->data);

                void* handle = dlopen(required_filename->data, RTLD_LAZY);
                if (!handle) {
                    do_error(extension->data, winsz.ws_row, winsz.ws_col);
                    free_char(extension);
                    free_char(required_filename);
                    continue;
                }

                void (*func)(const char*) = dlsym(handle, "open_file");
                if (!func) {
                    do_error("Could not find the required function in lib", winsz.ws_row, winsz.ws_col);
                    free_char(extension);
                    free_char(required_filename);
                    continue;
                }

                vector_char* filename = concat_path(current_path, current->data[shift + cursor_line - 1].name);

                (*func)(filename->data);

                free_char(extension);
                free_char(filename);
                free_char(required_filename);
            }
        }
        else if (c == 'c') {
            if (current->data[shift + cursor_line - 1].type == DT_DIR) {
                do_error("Directory copying is not supported, sorry :(", winsz.ws_row, winsz.ws_col);
                continue;
            }
            if (saved_fd_for_copy != -1) {
                free_char(saved_filename_for_copy);
                free_char(saved_full_path_for_copy);
                close(saved_fd_for_copy);
            }
            need_to_delete_source_for_copy = false;
            vector_char* name_of_file = concat_path(current_path, current->data[shift + cursor_line - 1].name);
            errno = 0;
            saved_fd_for_copy = open(name_of_file->data, O_RDONLY | O_LARGEFILE);

            if (saved_fd_for_copy < 0) {
                saved_fd_for_copy = -1;
                vector_char* msg = create_char(winsz.ws_col, '\0');
                snprintf(msg->data, msg->size, "%s : %s", "Failed to initiize copying of the file", strerror(errno));
                do_error(msg->data, winsz.ws_row, winsz.ws_col);
                free_char(msg);
                free_char(name_of_file);
                continue;
            }
            saved_full_path_for_copy = create_char(name_of_file->size, '\0');
            memcpy(saved_full_path_for_copy->data, name_of_file->data, name_of_file->size);

            saved_filename_for_copy = create_char(sizeof(current->data[shift + cursor_line - 1].name), '\0');
            memcpy(saved_filename_for_copy->data, current->data[shift + cursor_line - 1].name, saved_filename_for_copy->size);

            struct stat st;
            stat(name_of_file->data, &st);
            saved_mode_for_copy = st.st_mode;

            free_char(name_of_file);
        }
        else if (c == 'x') {
            if (current->data[shift + cursor_line - 1].type == DT_DIR) {
                do_error("Directory cutting is not supported, sorry :(", winsz.ws_row, winsz.ws_col);
                continue;
            }
            if (saved_fd_for_copy != -1) {
                free_char(saved_filename_for_copy);
                free_char(saved_full_path_for_copy);
                close(saved_fd_for_copy);
            }
            need_to_delete_source_for_copy = true;
            vector_char* name_of_file = concat_path(current_path, current->data[shift + cursor_line - 1].name);
            errno = 0;
            saved_fd_for_copy = open(name_of_file->data, O_RDONLY | O_LARGEFILE);

            if (saved_fd_for_copy < 0) {
                saved_fd_for_copy = -1;
                vector_char* msg = create_char(winsz.ws_col, '\0');
                snprintf(msg->data, msg->size, "%s : %s", "Failed to initiize cutting of the file", strerror(errno));
                do_error(msg->data, winsz.ws_row, winsz.ws_col);
                free_char(msg);
                free_char(name_of_file);
                continue;
            }
            saved_full_path_for_copy = create_char(name_of_file->size, '\0');
            memcpy(saved_full_path_for_copy->data, name_of_file->data, name_of_file->size);

            saved_filename_for_copy = create_char(sizeof(current->data[shift + cursor_line - 1].name), '\0');
            memcpy(saved_filename_for_copy->data, current->data[shift + cursor_line - 1].name, saved_filename_for_copy->size);

            struct stat st;
            stat(name_of_file->data, &st);
            saved_mode_for_copy = st.st_mode;

            free_char(name_of_file);
        }
        else if (c == 'v') {
            if (saved_fd_for_copy == -1) {
                do_error("Failed to insert a file : No file currently copied", winsz.ws_row, winsz.ws_col);
                continue;
            }
            vector_char* name_of_file = concat_path(current_path, saved_filename_for_copy->data);
            if (need_to_delete_source_for_copy) {
                errno = 0;
                int ret = rename(saved_full_path_for_copy->data, name_of_file->data);
                if (ret != 0) {
                    vector_char* msg = create_char(winsz.ws_col, '\0');
                    snprintf(msg->data, msg->size, "%s : %s", "Failed to rename a file", strerror(errno) /*saved_full_path_for_copy->data, name_of_file->data*/);
                    do_error(msg->data, winsz.ws_row, winsz.ws_col);
                    free_char(msg);
                }
                free_char(name_of_file);
                close(saved_fd_for_copy);
                free_char(saved_filename_for_copy);
                free_char(saved_full_path_for_copy);
                need_to_delete_source_for_copy = false;
                saved_filename_for_copy = NULL;
                saved_full_path_for_copy = NULL;
                saved_fd_for_copy = -1;
                switched_folders = true;
            }
            else {
                errno = 0;
                lseek64(saved_fd_for_copy, 0, SEEK_SET);
                int fd_to_write = open(name_of_file->data, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE, saved_mode_for_copy);

                if (fd_to_write < 0) {
                    vector_char* msg = create_char(winsz.ws_col, '\0');
                    snprintf(msg->data, msg->size, "%s : %s", "Failed to write a file to ", strerror(errno));
                    do_error(msg->data, winsz.ws_row, winsz.ws_col);
                    free_char(msg);
                    free_char(name_of_file);
                    continue;
                }

                static char buffer[4096];

                int to_write;
                while ((to_write = read(saved_fd_for_copy, buffer, 4096)) > 0) {
                    int written = 0;
                    do {
                        written += write(fd_to_write, buffer + written, to_write - written);
                    } while (written != to_write);
                }
                if (to_write < 0) {
                    do_error("Failed to write", winsz.ws_row, winsz.ws_col);
                    free_char(name_of_file);
                    continue;
                }

                if (close(fd_to_write) != 0) {
                    do_error("Failed to finalize insertion", winsz.ws_row, winsz.ws_col);
                    free_char(name_of_file);
                    continue;
                }
                switched_folders = true;
                free_char(name_of_file);
            }
        }
        else if (c == '\e') {
            if (!read_char_fd(input_fd, &c)) { // [ symbol is useless
                break;
            }
            if (!read_char_fd(input_fd, &c)) {
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
        else if (c == 'h') {
            if (filter_out_hidden_files) {
                filter_out_hidden_files = false;
                if (had_files_to_drop) {
                    force_redraw = true;
                }
            }
            else {
                filter_out_hidden_files = true;
                for (int i = 0; i < current->size; ++i) {
                    if (is_one_dot(current->data[i].name) || is_two_dots(current->data[i].name)) {
                        continue;
                    }
                    if (current->data[i].name[0] == '.') {
                        force_redraw = true;
                        break;
                    }
                }
            }
            if (force_redraw) {
                cursor_line = 1;
            }
            moved_cursor = true;
        }
        else {
            moved_cursor = true;
        }
    }

    free_char(saved_full_path_for_copy);
    free_char(saved_filename_for_copy);
    free_DirectoryEntry(current);
    free(current_path);
}
