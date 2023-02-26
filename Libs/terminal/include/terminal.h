#pragma once

#define _DEFAULT_SOURCE

#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "vector.h"

typedef struct DirectoryEntry {
    char name[256];
    unsigned char type;
    char mode[11];
    int64_t size;
} DirectoryEntry;
typedef const char* string;

VECTOR(string);
VECTOR(DirectoryEntry);
VECTOR(char);

bool get_terminal_size(struct winsize* result);

vector_DirectoryEntry* load_directory(const char* path);

vector_char* form_line(const char* parent_path, const DirectoryEntry* entry);

void sort_directory_entries(vector_DirectoryEntry* entries);
