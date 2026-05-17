// htree.c - Trien khai HTree (ext4 Hashed B-Tree) cho directory indexing
// Mo phong cach ext4 filesystem su dung HTree de index thu muc lon:
//   1. Tinh hash cua filename (Half-MD4)
//   2. Binary search trong dx_entries de tim leaf block phu hop
//   3. Linear scan trong leaf block de tim entry chinh xac
// Split leaf block khi day (tuong tu ext4 dx_make_map + split)

#include "htree.h"

// ===================================================================
// Hash Functions - Mo phong cac hash versions cua ext4
// ===================================================================

// half_md4_transform - Mo phong thuat toan Half-MD4 cua ext4
// Dua tren fs/ext4/hash.c trong Linux kernel
// Dung cac phep toan bitwise va mixing tuong tu MD4 nhung don gian hon
static void half_md4_transform(uint32_t buf[4], const uint32_t in[8]) {
    uint32_t a = buf[0], b = buf[1], c = buf[2], d = buf[3];

    // Round 1 - F(x,y,z) = (x & y) | (~x & z)
    #define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
    #define ROUND1(a, b, c, d, k, s) \
        (a) += F((b), (c), (d)) + (in[(k)]); \
        (a) = ((a) << (s)) | ((a) >> (32 - (s)));

    ROUND1(a, b, c, d, 0,  3);  ROUND1(d, a, b, c, 1,  7);
    ROUND1(c, d, a, b, 2, 11);  ROUND1(b, c, d, a, 3, 19);
    ROUND1(a, b, c, d, 4,  3);  ROUND1(d, a, b, c, 5,  7);
    ROUND1(c, d, a, b, 6, 11);  ROUND1(b, c, d, a, 7, 19);
    #undef ROUND1
    #undef F

    // Round 2 - G(x,y,z) = (x & y) | (x & z) | (y & z)
    #define G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
    #define ROUND2(a, b, c, d, k, s) \
        (a) += G((b), (c), (d)) + (in[(k)]) + 0x5A827999u; \
        (a) = ((a) << (s)) | ((a) >> (32 - (s)));

    ROUND2(a, b, c, d, 1,  3);  ROUND2(d, a, b, c, 3,  5);
    ROUND2(c, d, a, b, 5,  9);  ROUND2(b, c, d, a, 7, 13);
    ROUND2(a, b, c, d, 0,  3);  ROUND2(d, a, b, c, 2,  5);
    ROUND2(c, d, a, b, 4,  9);  ROUND2(b, c, d, a, 6, 13);
    #undef ROUND2
    #undef G

    // Round 3 - H(x,y,z) = x ^ y ^ z
    #define H(x, y, z) ((x) ^ (y) ^ (z))
    #define ROUND3(a, b, c, d, k, s) \
        (a) += H((b), (c), (d)) + (in[(k)]) + 0x6ED9EBA1u; \
        (a) = ((a) << (s)) | ((a) >> (32 - (s)));

    ROUND3(a, b, c, d, 3,  3);  ROUND3(d, a, b, c, 7,  9);
    ROUND3(c, d, a, b, 2, 11);  ROUND3(b, c, d, a, 6, 15);
    ROUND3(a, b, c, d, 1,  3);  ROUND3(d, a, b, c, 5,  9);
    ROUND3(c, d, a, b, 0, 11);  ROUND3(b, c, d, a, 4, 15);
    #undef ROUND3
    #undef H

    buf[0] += a;  buf[1] += b;
    buf[2] += c;  buf[3] += d;
}

// htree_half_md4 - Tinh hash Half-MD4 cho filename
// Mo phong ext4s_dx_hash (fs/ext4/hash.c) voi hash_version = DX_HASH_HALF_MD4
uint32_t htree_half_md4(const char* name) {
    uint32_t buf[4] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
    uint32_t in[8];
    int len = (int)strlen(name);

    // Pack name bytes vao mang in[] (4 bytes/word, little-endian)
    while (len > 0) {
        memset(in, 0, sizeof(in));
        for (int i = 0; i < 8 && len > 0; i++) {
            uint32_t val = 0;
            for (int j = 0; j < 4 && len > 0; j++, len--) {
                val |= ((uint32_t)(unsigned char)*name++) << (j * 8);
            }
            in[i] = val;
        }
        half_md4_transform(buf, in);
    }

    // Ket hop 2 thanh phan dau cua buf de tao hash 32-bit
    uint32_t hash = buf[1];
    // Loai bo gia tri 0 (reserved trong ext4)
    if (hash == 0) hash = 1;
    return hash;
}

// tea_hash - TEA (Tiny Encryption Algorithm) hash
// Mo phong ext4 TEA hash version
static uint32_t tea_hash(const char* name) {
    uint32_t k0 = 0x12345678, k1 = 0x9ABCDEF0;
    uint32_t sum = 0;
    uint32_t delta = 0x9E3779B9;
    int len = (int)strlen(name);

    // Pack name thanh 2 blocks
    uint32_t v0 = 0, v1 = 0;
    for (int i = 0; i < len; i++) {
        if (i < 4)
            v0 |= ((uint32_t)(unsigned char)name[i]) << (i * 8);
        else if (i < 8)
            v1 |= ((uint32_t)(unsigned char)name[i]) << ((i - 4) * 8);
    }

    // 16 rounds of TEA mixing
    for (int i = 0; i < 16; i++) {
        sum += delta;
        v0 += ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
        v1 += ((v0 << 4) + k1) ^ (v0 + sum) ^ ((v0 >> 5) + k0);
    }

    uint32_t hash = v0 ^ v1;
    if (hash == 0) hash = 1;
    return hash;
}

// compute_hash - Tinh hash dua tren version da chon
static uint32_t compute_hash(HTreeHashVersion version, const char* name) {
    switch (version) {
        case HTREE_HASH_HALF_MD4:
            return htree_half_md4(name);
        case HTREE_HASH_TEA:
            return tea_hash(name);
        case HTREE_HASH_DJB2: {
            // Reuse djb2 tu hash_table.h
            uint32_t hash = 5381;
            int c;
            const char* s = name;
            while ((c = (unsigned char)*s++)) {
                hash = ((hash << 5) + hash) + c;
            }
            if (hash == 0) hash = 1;
            return hash;
        }
        default:
            return htree_half_md4(name);
    }
}

// ===================================================================
// Internal Helpers
// ===================================================================

// create_leaf_block - Cap phat leaf block moi
static HTreeLeafBlock* create_leaf_block(void) {
    HTreeLeafBlock* block = (HTreeLeafBlock*)calloc(1, sizeof(HTreeLeafBlock));
    return block;
}

// grow_dx_entries - Nhan doi dung luong mang dx_entries khi can
static int grow_dx_entries(HTree* ht) {
    int new_cap = ht->dx_capacity * 2;
    HTreeDxEntry* new_arr = (HTreeDxEntry*)realloc(
        ht->dx_entries, (size_t)new_cap * sizeof(HTreeDxEntry));
    if (!new_arr) {
        fprintf(stderr, "Error: Cannot grow HTree dx_entries\n");
        return -1;
    }
    ht->dx_entries = new_arr;
    ht->dx_capacity = new_cap;
    return 0;
}

// find_dx_entry_index - Binary search trong mang dx_entries de tim
// leaf block phu hop cho hash value
// Tra ve index cua dx_entry co hash lon nhat <= target_hash
static int find_dx_entry_index(const HTree* ht, uint32_t target_hash,
                                uint64_t* comparisons) {
    if (ht->num_dx_entries == 0)
        return -1;

    // Binary search: tim dx_entry co hash lon nhat <= target_hash
    int lo = 0, hi = ht->num_dx_entries - 1;
    int result = 0; // Mac dinh la dx_entry dau tien

    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (comparisons) (*comparisons)++;

        if (ht->dx_entries[mid].hash <= target_hash) {
            result = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }

    return result;
}

// split_leaf_block - Tach leaf block khi day
// Tao leaf block moi, chuyen 1 nua entries co hash lon hon sang
// Cap nhat dx_entries de tro toi ca 2 blocks
static int split_leaf_block(HTree* ht, int dx_idx) {
    // Kiem tra va mo rong mang dx_entries neu can
    if (ht->num_dx_entries >= ht->dx_capacity) {
        if (grow_dx_entries(ht) != 0)
            return -1;
    }

    HTreeLeafBlock* old_block = ht->dx_entries[dx_idx].block;

    // Tinh hash cho tat ca entries trong old_block
    uint32_t hashes[HTREE_BLOCK_CAPACITY];
    for (int i = 0; i < old_block->count; i++) {
        hashes[i] = compute_hash(ht->hash_version, old_block->entries[i].name);
    }

    // Sap xep entries theo hash (insertion sort - block nho nen O(k^2) ok)
    for (int i = 1; i < old_block->count; i++) {
        uint32_t key_hash = hashes[i];
        DirEntry key_entry = old_block->entries[i];
        int j = i - 1;
        while (j >= 0 && hashes[j] > key_hash) {
            hashes[j + 1] = hashes[j];
            old_block->entries[j + 1] = old_block->entries[j];
            j--;
        }
        hashes[j + 1] = key_hash;
        old_block->entries[j + 1] = key_entry;
    }

    // Tach: nua dau o lai old_block, nua sau sang new_block
    int mid = old_block->count / 2;

    HTreeLeafBlock* new_block = create_leaf_block();
    if (!new_block)
        return -1;

    int new_count = old_block->count - mid;
    for (int i = 0; i < new_count; i++) {
        new_block->entries[i] = old_block->entries[mid + i];
    }
    new_block->count = new_count;
    old_block->count = mid;

    // Hash cua entry dau tien trong new_block la hash cho dx_entry moi
    uint32_t split_hash = hashes[mid];

    // Dich phai dx_entries de chen dx_entry moi
    for (int i = ht->num_dx_entries - 1; i > dx_idx; i--) {
        ht->dx_entries[i + 1] = ht->dx_entries[i];
    }
    ht->dx_entries[dx_idx + 1].hash = split_hash;
    ht->dx_entries[dx_idx + 1].block = new_block;
    ht->num_dx_entries++;

    return 0;
}

// ===================================================================
// Public API
// ===================================================================

// htree_create - Khoi tao HTree voi 1 leaf block ban dau
HTree* htree_create(void) {
    HTree* ht = (HTree*)calloc(1, sizeof(HTree));
    if (!ht) {
        fprintf(stderr, "Error: Cannot allocate HTree\n");
        return NULL;
    }

    ht->hash_version = HTREE_HASH_HALF_MD4;
    ht->depth = 0;
    ht->count = 0;

    // Cap phat mang dx_entries dong
    ht->dx_capacity = HTREE_INITIAL_DX_CAPACITY;
    ht->dx_entries = (HTreeDxEntry*)calloc(
        ht->dx_capacity, sizeof(HTreeDxEntry));
    if (!ht->dx_entries) {
        free(ht);
        return NULL;
    }

    // Tao leaf block dau tien voi hash = 0 (nhan tat ca entries ban dau)
    ht->num_dx_entries = 1;
    ht->dx_entries[0].hash = 0;
    ht->dx_entries[0].block = create_leaf_block();
    if (!ht->dx_entries[0].block) {
        free(ht->dx_entries);
        free(ht);
        return NULL;
    }

    return ht;
}

// htree_insert - Them entry vao HTree
// 1. Hash(name) -> tim dx_entry phu hop (binary search)
// 2. Insert entry vao leaf block
// 3. Split neu leaf block day
int htree_insert(HTree* ht, const DirEntry* entry) {
    if (!ht || !entry)
        return -1;

    uint32_t hash = compute_hash(ht->hash_version, entry->name);

    // Tim dx_entry phu hop
    int dx_idx = find_dx_entry_index(ht, hash, NULL);
    if (dx_idx < 0)
        return -1;

    HTreeLeafBlock* block = ht->dx_entries[dx_idx].block;

    // Kiem tra leaf block co day khong
    if (block->count >= HTREE_SPLIT_THRESHOLD) {
        // Split leaf block
        if (split_leaf_block(ht, dx_idx) != 0)
            return -1;

        // Tim lai dx_entry sau khi split
        dx_idx = find_dx_entry_index(ht, hash, NULL);
        block = ht->dx_entries[dx_idx].block;
    }

    // Them entry vao cuoi leaf block
    block->entries[block->count] = *entry;
    block->count++;
    ht->count++;

    return 0;
}

// htree_lookup - Tim kiem entry trong HTree
// 1. Hash(name) -> binary search dx_entries -> tim leaf block
// 2. Linear scan trong leaf block (giong ext4)
DirEntry* htree_lookup(HTree* ht, const char* name, uint64_t* comparisons) {
    if (!ht || !name)
        return NULL;

    uint64_t comp = 0;
    uint32_t hash = compute_hash(ht->hash_version, name);

    // Binary search trong dx_entries
    int dx_idx = find_dx_entry_index(ht, hash, &comp);
    if (dx_idx < 0) {
        if (comparisons) *comparisons = comp;
        return NULL;
    }

    // Linear scan trong leaf block (giong cach ext4 xu ly trong 1 block)
    HTreeLeafBlock* block = ht->dx_entries[dx_idx].block;
    for (int i = 0; i < block->count; i++) {
        comp++;
        if (strcmp(block->entries[i].name, name) == 0) {
            if (comparisons) *comparisons = comp;
            return &block->entries[i];
        }
    }

    // Hash collision: co the entry nam o block ke tiep (hash giong nhau)
    // Kiem tra block ke tiep neu ton tai
    if (dx_idx + 1 < ht->num_dx_entries) {
        HTreeLeafBlock* next_block = ht->dx_entries[dx_idx + 1].block;
        for (int i = 0; i < next_block->count; i++) {
            comp++;
            if (strcmp(next_block->entries[i].name, name) == 0) {
                if (comparisons) *comparisons = comp;
                return &next_block->entries[i];
            }
        }
    }

    if (comparisons) *comparisons = comp;
    return NULL;
}

// htree_delete - Xoa entry tu HTree
// 1. Hash(name) -> tim leaf block
// 2. Tim entry trong leaf block va xoa (swap voi phan tu cuoi)
int htree_delete(HTree* ht, const char* name, uint64_t* comparisons) {
    if (!ht || !name)
        return -1;

    uint64_t comp = 0;
    uint32_t hash = compute_hash(ht->hash_version, name);

    int dx_idx = find_dx_entry_index(ht, hash, &comp);
    if (dx_idx < 0) {
        if (comparisons) *comparisons = comp;
        return -1;
    }

    // Tim va xoa trong leaf block
    HTreeLeafBlock* block = ht->dx_entries[dx_idx].block;
    for (int i = 0; i < block->count; i++) {
        comp++;
        if (strcmp(block->entries[i].name, name) == 0) {
            // Swap voi phan tu cuoi roi giam count (O(1) delete trong block)
            block->entries[i] = block->entries[block->count - 1];
            block->count--;
            ht->count--;
            if (comparisons) *comparisons = comp;
            return 0;
        }
    }

    // Kiem tra block ke tiep (hash collision)
    if (dx_idx + 1 < ht->num_dx_entries) {
        HTreeLeafBlock* next_block = ht->dx_entries[dx_idx + 1].block;
        for (int i = 0; i < next_block->count; i++) {
            comp++;
            if (strcmp(next_block->entries[i].name, name) == 0) {
                next_block->entries[i] = next_block->entries[next_block->count - 1];
                next_block->count--;
                ht->count--;
                if (comparisons) *comparisons = comp;
                return 0;
            }
        }
    }

    if (comparisons) *comparisons = comp;
    return -1;
}

// htree_memory_usage - Tong bo nho = struct + dx_entries array + cac leaf blocks
size_t htree_memory_usage(const HTree* ht) {
    if (!ht)
        return 0;

    size_t usage = sizeof(HTree);
    usage += (size_t)ht->dx_capacity * sizeof(HTreeDxEntry);
    for (int i = 0; i < ht->num_dx_entries; i++) {
        if (ht->dx_entries[i].block) {
            usage += sizeof(HTreeLeafBlock);
        }
    }
    return usage;
}

// htree_stats - In thong ke HTree
void htree_stats(const HTree* ht) {
    if (!ht)
        return;

    int total_entries = 0;
    int min_block = HTREE_BLOCK_CAPACITY;
    int max_block = 0;
    int empty_blocks = 0;

    for (int i = 0; i < ht->num_dx_entries; i++) {
        HTreeLeafBlock* block = ht->dx_entries[i].block;
        if (block) {
            total_entries += block->count;
            if (block->count < min_block) min_block = block->count;
            if (block->count > max_block) max_block = block->count;
            if (block->count == 0) empty_blocks++;
        }
    }

    double avg_fill = ht->num_dx_entries > 0
        ? (double)total_entries / ht->num_dx_entries
        : 0.0;

    const char* hash_name;
    switch (ht->hash_version) {
        case HTREE_HASH_HALF_MD4: hash_name = "Half-MD4"; break;
        case HTREE_HASH_TEA:      hash_name = "TEA"; break;
        case HTREE_HASH_DJB2:     hash_name = "djb2"; break;
        default:                  hash_name = "Unknown"; break;
    }

    printf("  HTree Statistics:\n");
    printf("  ─────────────────────────────────────────────\n");
    printf("  Total Entries:  %d\n", ht->count);
    printf("  Hash Version:   %s\n", hash_name);
    printf("  Tree Depth:     %d\n", ht->depth);
    printf("  Leaf Blocks:    %d\n", ht->num_dx_entries);
    printf("  DX Capacity:    %d\n", ht->dx_capacity);
    printf("  Block Capacity: %d entries/block\n", HTREE_BLOCK_CAPACITY);
    printf("  Min Block Fill: %d\n", ht->num_dx_entries > 0 ? min_block : 0);
    printf("  Max Block Fill: %d\n", max_block);
    printf("  Avg Block Fill: %.1f\n", avg_fill);
    printf("  Empty Blocks:   %d\n", empty_blocks);
    printf("  Memory Usage:   %zu KB\n", htree_memory_usage(ht) / 1024);
}

// htree_destroy - Giai phong toan bo bo nho
void htree_destroy(HTree* ht) {
    if (!ht)
        return;

    for (int i = 0; i < ht->num_dx_entries; i++) {
        if (ht->dx_entries[i].block) {
            free(ht->dx_entries[i].block);
        }
    }
    free(ht->dx_entries);
    free(ht);
}
