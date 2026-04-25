// hash_table.h - Mo phong directory indexing cau truc Hash Table (O(1) avg)
// Su dung Separate Chaining (Singly Linked List) de xu ly Collision
// Rehash tu dong khi load_factor > 0.7

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "dir_entry.h"

// Load factor threshold - khi vuot qua nguong nay se rehash x2
#define HASH_LOAD_FACTOR_THRESHOLD  0.7

// Kich thuoc khoi tao mac dinh (so luot buckets)
#define HASH_INITIAL_CAPACITY  256

// HashNode - Mot node trong chuoi Linked List (Separate Chaining)
typedef struct HashNode {
    DirEntry          entry;
    struct HashNode*  next;
} HashNode;

// HashTable - Bang bam voi mang buckets va metadata
typedef struct {
    HashNode**  buckets;       // Mang con tro toi head cua moi chain
    int         num_buckets;   // Kich thuoc mang buckets hien tai
    int         count;         // Tong so entries da insert
} HashTable;

// djb2_hash - Ham bam djb2 cho chuoi filename (Daniel J. Bernstein)
uint32_t djb2_hash(const char* str);

// hash_create - Khoi tao Hash Table voi initial capacity
HashTable* hash_create(void);

// hash_insert - Them entry moi vao bang bam (auto rehash khi qua tai)
int hash_insert(HashTable* ht, const DirEntry* entry);

// hash_lookup - Tim kiem entry theo ten (O(1) trung binh)
DirEntry* hash_lookup(HashTable* ht, const char* name, uint64_t* comparisons);

// hash_delete - Xoa entry theo ten tu chain
int hash_delete(HashTable* ht, const char* name, uint64_t* comparisons);

// hash_memory_usage - Tinh tong bo nho da cap phat
size_t hash_memory_usage(const HashTable* ht);

// hash_destroy - Giai phong toan bo bo nho (buckets + nodes)
void hash_destroy(HashTable* ht);

// hash_load_factor - Tra ve ti le load hien tai
double hash_load_factor(const HashTable* ht);

// hash_stats - In thong ke phan bo buckets (debug)
void hash_stats(const HashTable* ht);

#endif /* HASH_TABLE_H */
