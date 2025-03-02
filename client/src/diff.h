#include <time.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_PSEUDONAME_LENGTH 11
#define MAX_FILE_CONTENT_LENGTH 1024
#define MAX_FILES 10

typedef struct {
    char pseudo_name[MAX_PSEUDONAME_LENGTH];
    char real_name[MAX_FILENAME_LENGTH];
    char content[MAX_FILE_CONTENT_LENGTH];
    int version;
    time_t last_modified;
    int changed;
} PseudoFile;

extern PseudoFile* files[MAX_FILES];
extern int file_count;

char* generate_random_filename();
PseudoFile* create_pseudo_file(const char* filename, const char* content);
void update_pseudo_file(PseudoFile* file, const char* new_content);
PseudoFile* find_file(const char* filename);

