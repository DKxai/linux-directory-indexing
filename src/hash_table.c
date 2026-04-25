// hash_table.c - Trien khai Hash Table voi Separate Chaining va Auto-Rehash

#include "hash_table.h"

// =============================================================
// djb2_hash - Hash function cua Daniel J. Bernstein
// Cong thuc: hash = hash * 33 + c
// Duoc chon vi: don gian, phan bo deu, hieu qua cho chuoi ngan
// =============================================================
uint32_t djb2_hash(const char* str) {
    uint32_t hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return hash;
}

// hash_create - Cap phat bang bam voi HASH_INITIAL_CAPACITY buckets
HashTable* hash_create(void) {
    HashTable* ht = (HashTable*)malloc(sizeof(HashTable));
    if (!ht) {
        fprintf(stderr, "Error: Cannot allocate HashTable\n");
        return NULL;
    }

    ht->num_buckets = HASH_INITIAL_CAPACITY;
    ht->count = 0;

    // Calloc tu dong zero-init tat ca bucket pointers = NULL
    ht->buckets = (HashNode**)calloc(ht->num_buckets, sizeof(HashNode*));
    if (!ht->buckets) {
        fprintf(stderr, "Error: Cannot allocate %d buckets\n", ht->num_buckets);
        free(ht);
        return NULL;
    }

    return ht;
}

// hash_rehash (static) - Tang gap doi so buckets va phan bo lai entries
// Duoc goi tu dong khi load_factor > HASH_LOAD_FACTOR_THRESHOLD
static int hash_rehash(HashTable* ht) {
    int old_num_buckets = ht->num_buckets;
    HashNode** old_buckets = ht->buckets;

    int new_num_buckets = old_num_buckets * 2;
    HashNode** new_buckets = (HashNode**)calloc(new_num_buckets, sizeof(HashNode*));
    if (!new_buckets) {
        fprintf(stderr, "Error: Cannot allocate %d buckets for rehash\n", new_num_buckets);
        return -1;
    }

    // Di chuyen tung node tu bang cu sang bang moi
    for (int i = 0; i < old_num_buckets; i++) {
        HashNode* current = old_buckets[i];
        while (current) {
            HashNode* next = current->next;

            // Tinh lai index tren bang moi
            uint32_t idx = djb2_hash(current->entry.name) % new_num_buckets;
            current->next = new_buckets[idx];
            new_buckets[idx] = current;

            current = next;
        }
    }

    // Giai phong mang buckets cu (cac node da duoc chuyen sang)
    free(old_buckets);

    ht->buckets = new_buckets;
    ht->num_buckets = new_num_buckets;

    return 0;
}

// hash_insert - Them entry vao bucket tuong ung, rehash neu qua tai
int hash_insert(HashTable* ht, const DirEntry* entry) {
    if (!ht || !entry)
        return -1;

    // Kiem tra load factor truoc khi insert
    double lf = (double)(ht->count + 1) / (double)ht->num_buckets;
    if (lf > HASH_LOAD_FACTOR_THRESHOLD) {
        if (hash_rehash(ht) != 0)
            return -1;
    }

    // Tinh bucket index
    uint32_t idx = djb2_hash(entry->name) % ht->num_buckets;

    // Tao node moi
    HashNode* node = (HashNode*)malloc(sizeof(HashNode));
    if (!node) {
        fprintf(stderr, "Error: Cannot allocate HashNode\n");
        return -1;
    }

    // Copy entry data vao node
    node->entry = *entry;

    // Chen vao dau chain (O(1) prepend)
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;

    ht->count++;
    return 0;
}

// hash_lookup - Tim entry theo ten, dem so comparisons
DirEntry* hash_lookup(HashTable* ht, const char* name, uint64_t* comparisons) {
    if (!ht || !name)
        return NULL;

    uint64_t comp = 0;

    uint32_t idx = djb2_hash(name) % ht->num_buckets;

    // Duyet chain tai bucket[idx]
    HashNode* current = ht->buckets[idx];
    while (current) {
        comp++;
        if (strcmp(current->entry.name, name) == 0) {
            if (comparisons)
                *comparisons = comp;
            return &current->entry;
        }
        current = current->next;
    }

    if (comparisons)
        *comparisons = comp;
    return NULL; // Khong tim thay
}

// hash_delete - Tim va xoa node tu chain
int hash_delete(HashTable* ht, const char* name, uint64_t* comparisons) {
    if (!ht || !name)
        return -1;

    uint64_t comp = 0;

    uint32_t idx = djb2_hash(name) % ht->num_buckets;

    HashNode* current = ht->buckets[idx];
    HashNode* prev = NULL;

    while (current) {
        comp++;
        if (strcmp(current->entry.name, name) == 0) {
            // Tim thay: cap nhat linked list
            if (prev) {
                prev->next = current->next;
            } else {
                // Node dau chain
                ht->buckets[idx] = current->next;
            }
            free(current);
            ht->count--;

            if (comparisons)
                *comparisons = comp;
            return 0; // Xoa thanh cong
        }
        prev = current;
        current = current->next;
    }

    if (comparisons)
        *comparisons = comp;
    return -1; // Khong tim thay
}

// hash_memory_usage - Tong memory = struct + buckets array + N nodes
size_t hash_memory_usage(const HashTable* ht) {
    if (!ht)
        return 0;

    size_t usage = sizeof(HashTable);
    usage += (size_t)ht->num_buckets * sizeof(HashNode*);
    usage += (size_t)ht->count * sizeof(HashNode);

    return usage;
}

// hash_load_factor - Ti le giua so entries / so buckets
double hash_load_factor(const HashTable* ht) {
    if (!ht || ht->num_buckets == 0)
        return 0.0;
    return (double)ht->count / (double)ht->num_buckets;
}

// hash_stats - In thong ke phan bo cho debug
void hash_stats(const HashTable* ht) {
    if (!ht)
        return;

    int empty_buckets = 0;
    int max_chain = 0;
    int total_chain = 0;

    for (int i = 0; i < ht->num_buckets; i++) {
        int chain_len = 0;
        HashNode* current = ht->buckets[i];
        while (current) {
            chain_len++;
            current = current->next;
        }
        if (chain_len == 0)
            empty_buckets++;
        if (chain_len > max_chain)
            max_chain = chain_len;
        total_chain += chain_len;
    }

    printf("  Hash Table Statistics:\n");
    printf("  ─────────────────────────────────────────────\n");
    printf("  Entries:        %d\n", ht->count);
    printf("  Buckets:        %d\n", ht->num_buckets);
    printf("  Load Factor:    %.3f\n", hash_load_factor(ht));
    printf("  Empty Buckets:  %d (%.1f%%)\n",
           empty_buckets, 100.0 * empty_buckets / ht->num_buckets);
    printf("  Max Chain Len:  %d\n", max_chain);
    printf("  Avg Chain Len:  %.2f\n",
           ht->num_buckets > empty_buckets
               ? (double)total_chain / (ht->num_buckets - empty_buckets)
               : 0.0);
    printf("  Memory Usage:   %zu KB\n", hash_memory_usage(ht) / 1024);
}

// hash_destroy - Free tat ca nodes trong moi chain, roi free mang buckets
void hash_destroy(HashTable* ht) {
    if (!ht)
        return;

    for (int i = 0; i < ht->num_buckets; i++) {
        HashNode* current = ht->buckets[i];
        while (current) {
            HashNode* next = current->next;
            free(current);
            current = next;
        }
    }

    free(ht->buckets);
    free(ht);
}
