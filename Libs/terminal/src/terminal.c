#include "terminal.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool get_terminal_size(struct winsize* result) {
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, result) < 0) {
        return false;
    }
    return true;
}

bool readuser(int perms) {
    return (perms >> 8) & 1;
}

bool readgroup(int perms) {
    return (perms >> 5) & 1;
}

bool readall(int perms) {
    return (perms >> 2) & 1;
}

bool writeuser(int perms) {
    return (perms >> 7) & 1;
}

bool writegroup(int perms) {
    return (perms >> 4) & 1;
}

bool writeall(int perms) {
    return (perms >> 1) & 1;
}

bool executeuser(int perms) {
    return (perms >> 6) & 1;
}

bool executegroup(int perms) {
    return (perms >> 3) & 1;
}

bool executeall(int perms) {
    return (perms >> 0) & 1;
}

bool suid(int perms) {
    return (perms >> 11) & 1;
}

bool sgid(int perms) {
    return (perms >> 10) & 1;
}

bool sticky(int perms) {
    return (perms >> 9) & 1;
}

const char *perms_to_str(char *buf, size_t size, int perms) {
    for (size_t i = 0; i < size - 1; ++i) {
        buf[i] = '-';
    }
    buf[size - 1] = '\0';
    for (int i = 9; i < size; ++i) {
        buf[i] = '\0';
    }

    if (readuser(perms) && size >= 2) {
        buf[0] = 'r';
    }
    if (writeuser(perms) && size >= 3) {
        buf[1] = 'w';
    }
    if (executeuser(perms) && size >= 4) {
        buf[2] = 'x';
    }
    if (suid(perms) && (executegroup(perms) || executeall(perms))) {
        buf[2] = 's';
    }
    if (readgroup(perms) && size >= 5) {
        buf[3] = 'r';
    }
    if (writegroup(perms) && size >= 6) {
        buf[4] = 'w';
    }
    if (executegroup(perms) && size >= 7) {
        buf[5] = 'x';
    }
    if (sgid(perms) && executeall(perms)) {
        buf[5] = 's';
    }
    if (readall(perms) && size >= 8) {
        buf[6] = 'r';
    }
    if (writeall(perms) && size >= 9) {
        buf[7] = 'w';
    }
    if (executeall(perms) && size >= 10) {
        buf[8] = 'x';
    }
    if (sticky(perms) && executeall(perms) && writeall(perms)) {
        buf[8] = 't';
    }
    return buf;
}

vector_DirectoryEntry* load_directory(const char* path) {
    DIR* dir = opendir(path);
    struct dirent* current = NULL;
    vector_DirectoryEntry* entries = reserve_junk_DirectoryEntry(1);
    while (current = readdir(dir)) {
        create_back_DirectoryEntry(entries);
        back_DirectoryEntry(entries)->type = current->d_type;
        int parent_size = strlen(path);
        int entry_size = strlen(current->d_name);
        char* current_name = calloc(parent_size + entry_size + 2, 1);
        snprintf(current_name, parent_size + entry_size + 2, "%s/%s", path, current->d_name);
        struct stat st;
        stat(current_name, &st);
        (void) perms_to_str(back_DirectoryEntry(entries)->mode + 1, 10, st.st_mode);
        if (current->d_type == DT_DIR) {
            (back_DirectoryEntry(entries))->mode[0] = 'd';
        }
        else {
            (back_DirectoryEntry(entries))->mode[0] = '-';
        }
        memcpy(back_DirectoryEntry(entries)->name, current->d_name, 256);
        free(current_name);
    }

    closedir(dir);
    return entries;
}

vector_char* form_line(const char* parent_path, const DirectoryEntry* entry) {
    int parent_size = strlen(parent_path);
    int entry_size = strlen(entry->name);

    vector_char* line = create_char(11 + 1 + 4 + 5 + entry_size + 4 + 1, '\0');
    if (entry->type == DT_DIR) {
        snprintf(line->data, line->size - 1, "%s \e[1m\e[34m%s\e[0m", entry->mode, entry->name);
    }
    else if (entry->type == DT_REG) {
        if (entry->mode[3] == 'x' || entry->mode[6] == 'x' || entry->mode[9] == 'x') {
            snprintf(line->data, line->size - 1, "%s \e[1m\e[32m%s\e[0m", entry->mode, entry->name);
        }
        else {
            snprintf(line->data, line->size - 1, "%s %s", entry->mode, entry->name);
        }
    }
    else if (entry->type == DT_FIFO) {
        snprintf(line->data, line->size - 1, "%s \e[1m\e[35m%s\e[0m", entry->mode, entry->name);
    }
    else {
        snprintf(line->data, line->size - 1, "%s \e[1m\e[36m%s\e[0m", entry->mode, entry->name);
    }
    return line;
}

int compare_directory_entries(const void* a, const void* b) {
    return strcmp(((const DirectoryEntry*)(a))->name, ((const DirectoryEntry*)(b))->name);
}

void sort_directory_entries(vector_DirectoryEntry* entries) {
    qsort(entries->data, entries->size, sizeof(DirectoryEntry), compare_directory_entries);
}
