// btree.h - Mo phong directory indexing cau truc B-Tree (O(log n))
// Cay can bang da nhanh, toi uu cho truy cap disk theo block
// Moi node chua nhieu keys, giam chieu cao cay -> giam so truy cap

#ifndef BTREE_H
#define BTREE_H

#include "dir_entry.h"

// Bac toi thieu cua B-Tree (minimum degree)
// Moi node (tru root) co toi thieu t-1 keys va toi da 2t-1 keys
// t=50 -> moi node chua 49..99 keys, phu hop mo phong filesystem block
#define BTREE_MIN_DEGREE  50

// BTreeNode - Mot node trong B-Tree
typedef struct BTreeNode {
    DirEntry*           keys;       // Mang keys (DirEntry), toi da 2t-1
    struct BTreeNode**  children;   // Mang con tro den cac con, toi da 2t
    int                 num_keys;   // So keys hien tai trong node
    bool                is_leaf;    // Co phai la node la (leaf) khong
} BTreeNode;

// BTree - Cay B-Tree voi root va metadata
typedef struct {
    BTreeNode*  root;          // Con tro den node goc
    int         min_degree;    // Bac toi thieu t
    int         count;         // Tong so entries trong cay
} BTree;

// btree_create - Khoi tao B-Tree rong voi bac toi thieu t
BTree* btree_create(void);

// btree_insert - Them entry vao B-Tree (tu dong split khi node day)
int btree_insert(BTree* tree, const DirEntry* entry);

// btree_lookup - Tim kiem entry theo ten (O(log n))
// Su dung binary search trong tung node, de quy xuong con thich hop
DirEntry* btree_lookup(BTree* tree, const char* name, uint64_t* comparisons);

// btree_delete - Xoa entry theo ten tu B-Tree
// Xu ly cac truong hop: leaf, internal, borrow, merge
int btree_delete(BTree* tree, const char* name, uint64_t* comparisons);

// btree_memory_usage - Tinh tong bo nho da cap phat (de quy)
size_t btree_memory_usage(const BTree* tree);

// btree_destroy - Giai phong toan bo bo nho (de quy)
void btree_destroy(BTree* tree);

// btree_height - Tra ve chieu cao cua cay
int btree_height(const BTree* tree);

// btree_stats - In thong ke cay (debug)
void btree_stats(const BTree* tree);

#endif /* BTREE_H */
