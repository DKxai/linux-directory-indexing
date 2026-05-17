// htree.h - Mo phong ext4 HTree directory indexing
// HTree = Hashed B-Tree: ket hop hash va cay can bang
// Cau truc tuong tu ext4: dx_root -> dx_entry -> leaf blocks (linear scan)
// Lookup: hash(name) -> binary search dx_entries -> linear scan trong leaf block

#ifndef HTREE_H
#define HTREE_H

#include "dir_entry.h"

// So luong entries toi da trong moi leaf block
// Mo phong kich thuoc 1 disk block 4KB chua cac DirEntry
#define HTREE_BLOCK_CAPACITY  64

// Kich thuoc khoi tao cua mang dx_entries
#define HTREE_INITIAL_DX_CAPACITY  256

// Nguong split: khi leaf block day, tach thanh 2 block
#define HTREE_SPLIT_THRESHOLD  HTREE_BLOCK_CAPACITY

// Hash version - mo phong ext4 hash versions
typedef enum {
    HTREE_HASH_HALF_MD4 = 0,   // Half-MD4 (mac dinh cua ext4)
    HTREE_HASH_TEA      = 1,   // TEA (Tiny Encryption Algorithm)
    HTREE_HASH_DJB2     = 2    // djb2 (de so sanh voi Hash Table cua tuan 4)
} HTreeHashVersion;

// HTreeLeafBlock - Mot khoi la chua cac DirEntry (tuong tu ext4 leaf block)
// Trong khoi la, entries duoc luu tuan tu va duoc tim bang linear scan
typedef struct HTreeLeafBlock {
    DirEntry   entries[HTREE_BLOCK_CAPACITY];  // Mang cac entries
    int        count;                           // So entries hien tai
} HTreeLeafBlock;

// HTreeDxEntry - Index entry trong dx_root/dx_node
// Moi dx_entry anh xa mot khoang hash toi mot leaf block
typedef struct {
    uint32_t          hash;    // Gia tri hash nho nhat trong khoang
    HTreeLeafBlock*   block;   // Con tro toi leaf block tuong ung
} HTreeDxEntry;

// HTree - Cau truc HTree chinh (mo phong dx_root cua ext4)
// Gom mang dx_entries dong (sap xep theo hash) va metadata
typedef struct {
    HTreeDxEntry*     dx_entries;       // Mang index entries (dong)
    int               num_dx_entries;   // So dx_entries hien tai
    int               dx_capacity;     // Dung luong mang dx_entries
    int               count;            // Tong so DirEntry trong cay
    HTreeHashVersion  hash_version;     // Phien ban hash dang dung
    int               depth;            // Chieu sau cay (0 = chi co root)
} HTree;

// htree_create - Khoi tao HTree voi hash version cho truoc
HTree* htree_create(void);

// htree_insert - Them entry vao HTree (hash -> dx_entry -> leaf block)
// Tu dong split leaf block khi day
int htree_insert(HTree* ht, const DirEntry* entry);

// htree_lookup - Tim kiem entry theo ten
// Hash(name) -> binary search dx_entries -> linear scan trong leaf block
DirEntry* htree_lookup(HTree* ht, const char* name, uint64_t* comparisons);

// htree_delete - Xoa entry theo ten tu leaf block tuong ung
int htree_delete(HTree* ht, const char* name, uint64_t* comparisons);

// htree_memory_usage - Tinh tong bo nho da cap phat
size_t htree_memory_usage(const HTree* ht);

// htree_destroy - Giai phong toan bo bo nho
void htree_destroy(HTree* ht);

// htree_stats - In thong ke HTree (debug)
void htree_stats(const HTree* ht);

// htree_half_md4 - Ham hash Half-MD4 (mo phong ext4)
uint32_t htree_half_md4(const char* name);

#endif /* HTREE_H */
