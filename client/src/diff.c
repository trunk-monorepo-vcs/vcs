#include "diff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

PseudoFile* files[MAX_FILES]; // pseudofiles
int file_count = 1;

char* generate_random_filename() {
    char *pseudo_filename = (char*)malloc(MAX_PSEUDONAME_LENGTH * sizeof(char));

    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < MAX_PSEUDONAME_LENGTH - 1; i++) {
        pseudo_filename[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    pseudo_filename[MAX_PSEUDONAME_LENGTH - 1] = '\0';
    return pseudo_filename;
}

PseudoFile* create_pseudo_file(const char* filename, const char* content) {
    PseudoFile* file = (PseudoFile*)malloc(sizeof(PseudoFile));
    if (file == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    char* random_name = generate_random_filename();
    strncpy(file->pseudo_name, random_name, MAX_PSEUDONAME_LENGTH);
    free(random_name);
    strncpy(file->real_name, filename, MAX_FILENAME_LENGTH);
    strncpy(file->content, content, MAX_FILE_CONTENT_LENGTH);
    file->version = 1;
    file->changed = 0;
    file->last_modified = time(NULL);
    files[file_count++] = file;
    return file;
}

void update_pseudo_file(PseudoFile* file, const char* new_content) {
    if (file == NULL) {
        fprintf(stderr, "File pointer is NULL\n");
        return;
    }

    strncpy(file->content, new_content, MAX_FILE_CONTENT_LENGTH);
    file->version++;
    file->changed = 1;
    file->last_modified = time(NULL);
}

PseudoFile* find_file(const char* filename) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i]->real_name, filename) == 0) {
            return files[i];
        }
    }
    return NULL;
}
