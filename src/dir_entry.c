// dir_entry.c - Cac ham tao data test cho DirEntry

#include "dir_entry.h"

static const char* EXTENSIONS[] = {
    ".c", ".h", ".o", ".py", ".js", ".html", ".css", ".txt",
    ".md", ".log", ".conf", ".sh", ".json", ".xml", ".yaml",
    ".jpg", ".png", ".gif", ".pdf", ".zip", ".tar", ".gz",
    ".so", ".a", ".out", ".bin", ".dat", ".csv", ".sql", ".rs"
};
static const int NUM_EXTENSIONS = sizeof(EXTENSIONS) / sizeof(EXTENSIONS[0]);

static const char* PREFIXES[] = {
    "main", "utils", "config", "test", "module", "handler",
    "service", "model", "view", "controller", "helper", "data",
    "cache", "queue", "worker", "manager", "factory", "builder",
    "parser", "lexer", "driver", "kernel", "init", "setup",
    "auth", "user", "admin", "api", "db", "log", "core", "base"
};
static const int NUM_PREFIXES = sizeof(PREFIXES) / sizeof(PREFIXES[0]);

// generate_filename - Sinh file name random noi prefix voi extension
static void generate_filename(char* buf, int index) {
    const char* prefix = PREFIXES[rand() % NUM_PREFIXES];
    const char* ext = EXTENSIONS[rand() % NUM_EXTENSIONS];
    snprintf(buf, MAX_FILENAME_LEN, "%s_%06d%s", prefix, index, ext);
}

// generate_random_entries - Cap phat mang random Entries
DirEntry* generate_random_entries(int count) {
    DirEntry* entries = (DirEntry*)malloc(count * sizeof(DirEntry));
    if (!entries) {
        fprintf(stderr, "Error: Cannot allocate memory for %d entries\n", count);
        return NULL;
    }

    // Seed tinh de ket qua qua nhieu vong test ko bi nhieu ngau nhien
    srand(42); 

    for (int i = 0; i < count; i++) {
        entries[i].inode = (uint32_t)(i + 1);  
        entries[i].file_type = FT_REG_FILE;
        
        generate_filename(entries[i].name, i);
        entries[i].name_len = (uint8_t)strlen(entries[i].name);
        entries[i].rec_len = (uint16_t)(sizeof(DirEntry));
    }

    // Shuffle mang (Randomize) de check do phan manh
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        DirEntry temp = entries[i];
        entries[i] = entries[j];
        entries[j] = temp;
    }

    return entries;
}

// generate_lookup_hits - Lay mang name da ton tai
char** generate_lookup_hits(DirEntry* entries, int num_entries, int num_targets) {
    char** targets = (char**)malloc(num_targets * sizeof(char*));
    if (!targets) return NULL;

    for (int i = 0; i < num_targets; i++) {
        int idx = rand() % num_entries;
        targets[i] = strdup(entries[idx].name);
    }
    return targets;
}

// generate_lookup_misses - Tao mang name ko ton tai
char** generate_lookup_misses(int num_targets) {
    char** targets = (char**)malloc(num_targets * sizeof(char*));
    if (!targets) return NULL;

    for (int i = 0; i < num_targets; i++) {
        targets[i] = (char*)malloc(MAX_FILENAME_LEN + 1);
        // String xyz co dinh la duoi khong co trong mang EXTENSIONS
        snprintf(targets[i], MAX_FILENAME_LEN, "NONEXISTENT_%08d.xyz", i);
    }
    return targets;
}

// free_lookup_targets - Clean mang data test
void free_lookup_targets(char** targets, int count) {
    if (!targets) return;
    for (int i = 0; i < count; i++) {
        free(targets[i]);
    }
    free(targets);
}

// generate_delete_targets - Lay mang ten tu entries, dung de test delete
// Chon cac entry tu cuoi mang de tranh trung voi lookup hits
char** generate_delete_targets(DirEntry* entries, int num_entries, int num_targets) {
    if (num_targets > num_entries)
        num_targets = num_entries;

    char** targets = (char**)malloc(num_targets * sizeof(char*));
    if (!targets) return NULL;

    // Lay entries tu cuoi mang (phan khong trung voi lookup hits)
    for (int i = 0; i < num_targets; i++) {
        int idx = num_entries - 1 - (i % num_entries);
        targets[i] = strdup(entries[idx].name);
    }
    return targets;
}

// print_dir_entry - Print Dir object
void print_dir_entry(const DirEntry* entry) {
    const char* type_str;
    switch (entry->file_type) {
        case FT_REG_FILE: type_str = "FILE"; break;
        case FT_DIR:      type_str = "DIR "; break;
        case FT_SYMLINK:  type_str = "LINK"; break;
        default:          type_str = "UNKN"; break;
    }
    printf("  [%s] inode=%-8u name=%s\n", type_str, entry->inode, entry->name);
}
