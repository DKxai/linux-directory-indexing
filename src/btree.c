// btree.c - Trien khai B-Tree cho directory indexing
// B-Tree bac t: moi node co t-1..2t-1 keys (tru root co the co 1..2t-1)
// Lookup O(log n), Insert O(log n), Delete O(log n)

#include "btree.h"

// ===================================================================
// Internal helpers
// ===================================================================

// create_node - Cap phat node moi voi dung luong 2t-1 keys va 2t children
static BTreeNode* create_node(int t, bool is_leaf) {
    BTreeNode* node = (BTreeNode*)malloc(sizeof(BTreeNode));
    if (!node) return NULL;

    int max_keys = 2 * t - 1;

    node->keys = (DirEntry*)malloc(max_keys * sizeof(DirEntry));
    if (!node->keys) {
        free(node);
        return NULL;
    }

    // Leaf node khong can mang children, nhung cap phat de don gian hoa code
    node->children = (BTreeNode**)calloc(max_keys + 1, sizeof(BTreeNode*));
    if (!node->children) {
        free(node->keys);
        free(node);
        return NULL;
    }

    node->num_keys = 0;
    node->is_leaf = is_leaf;
    return node;
}

// destroy_node - Giai phong de quy tung node va cac con cua no
static void destroy_node(BTreeNode* node) {
    if (!node) return;

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            destroy_node(node->children[i]);
        }
    }

    free(node->keys);
    free(node->children);
    free(node);
}

// node_memory_usage - Tinh bo nho de quy cua mot node va toan bo cay con
static size_t node_memory_usage(const BTreeNode* node, int t) {
    if (!node) return 0;

    int max_keys = 2 * t - 1;
    size_t usage = sizeof(BTreeNode);
    usage += (size_t)max_keys * sizeof(DirEntry);           // keys array
    usage += (size_t)(max_keys + 1) * sizeof(BTreeNode*);   // children array

    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            usage += node_memory_usage(node->children[i], t);
        }
    }

    return usage;
}

// node_count - Dem tong so nodes trong cay (de quy)
static int node_count(const BTreeNode* node) {
    if (!node) return 0;

    int count = 1;
    if (!node->is_leaf) {
        for (int i = 0; i <= node->num_keys; i++) {
            count += node_count(node->children[i]);
        }
    }
    return count;
}

// ===================================================================
// Lookup - Tim kiem entry theo ten, dem comparisons
// Su dung linear search trong tung node (tuong tu filesystem thuc te)
// ===================================================================

// search_node - Tim kiem de quy tu mot node
static DirEntry* search_node(BTreeNode* node, const char* name,
                              uint64_t* comp) {
    if (!node) return NULL;

    // Tim vi tri key >= name trong node hien tai
    int i = 0;
    while (i < node->num_keys) {
        (*comp)++;
        int cmp = strcmp(name, node->keys[i].name);
        if (cmp == 0) {
            return &node->keys[i];  // Tim thay
        }
        if (cmp < 0) {
            break;  // name < keys[i] -> di xuong children[i]
        }
        i++;
    }

    // Neu la leaf thi khong tim thay
    if (node->is_leaf) {
        return NULL;
    }

    // De quy xuong con thich hop
    return search_node(node->children[i], name, comp);
}

// ===================================================================
// Insert - Them entry voi co che proactive split
// Split node day TRUOC khi di xuong con -> dam bao parent luon co cho
// ===================================================================

// split_child - Tach node children[idx] thanh 2 node khi day (2t-1 keys)
// Node cha (parent) nhan key giua, 2 node con chia deu keys
static void split_child(BTreeNode* parent, int idx, int t) {
    BTreeNode* full_child = parent->children[idx];
    
    // Tao node moi chua nua phai cua full_child
    BTreeNode* new_child = create_node(t, full_child->is_leaf);
    new_child->num_keys = t - 1;

    // Copy t-1 keys cuoi cua full_child sang new_child
    for (int j = 0; j < t - 1; j++) {
        new_child->keys[j] = full_child->keys[j + t];
    }

    // Neu khong phai leaf: copy t children cuoi sang new_child
    if (!full_child->is_leaf) {
        for (int j = 0; j < t; j++) {
            new_child->children[j] = full_child->children[j + t];
        }
    }

    full_child->num_keys = t - 1;

    // Dich phai children cua parent de chen new_child
    for (int j = parent->num_keys; j >= idx + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[idx + 1] = new_child;

    // Dich phai keys cua parent de chen key giua (median)
    for (int j = parent->num_keys - 1; j >= idx; j--) {
        parent->keys[j + 1] = parent->keys[j];
    }
    parent->keys[idx] = full_child->keys[t - 1];  // Key giua len parent
    parent->num_keys++;
}

// insert_nonfull - Chen key vao node chua day
// Neu leaf: chen truc tiep. Neu internal: tim con thich hop, split truoc neu can
static void insert_nonfull(BTreeNode* node, const DirEntry* entry, int t) {
    int i = node->num_keys - 1;

    if (node->is_leaf) {
        // Tim vi tri chen va dich phai cac key lon hon
        while (i >= 0 && strcmp(entry->name, node->keys[i].name) < 0) {
            node->keys[i + 1] = node->keys[i];
            i--;
        }
        node->keys[i + 1] = *entry;
        node->num_keys++;
    } else {
        // Tim con thich hop de di xuong
        while (i >= 0 && strcmp(entry->name, node->keys[i].name) < 0) {
            i--;
        }
        i++;

        // Proactive split: neu con day thi split truoc khi di xuong
        if (node->children[i]->num_keys == 2 * t - 1) {
            split_child(node, i, t);

            // Sau khi split, key giua da len parent
            // Xac dinh di vao con trai hay con phai
            if (strcmp(entry->name, node->keys[i].name) > 0) {
                i++;
            }
        }

        insert_nonfull(node->children[i], entry, t);
    }
}

// ===================================================================
// Delete - Xoa entry voi xu ly day du cac truong hop
// Borrow tu sibling hoac merge khi node con it keys
// ===================================================================

// find_key_index - Tim vi tri cua key trong node (tra ve index hoac vi tri chen)
static int find_key_index(BTreeNode* node, const char* name) {
    int idx = 0;
    while (idx < node->num_keys && strcmp(node->keys[idx].name, name) < 0) {
        idx++;
    }
    return idx;
}

// get_predecessor - Lay key lon nhat trong cay con trai (predecessor)
static DirEntry get_predecessor(BTreeNode* node) {
    while (!node->is_leaf) {
        node = node->children[node->num_keys];
    }
    return node->keys[node->num_keys - 1];
}

// get_successor - Lay key nho nhat trong cay con phai (successor)
static DirEntry get_successor(BTreeNode* node) {
    while (!node->is_leaf) {
        node = node->children[0];
    }
    return node->keys[0];
}

// merge_children - Gop children[idx] va children[idx+1] thanh 1 node
// Key parent->keys[idx] duoc keo xuong lam key giua
static void merge_children(BTreeNode* parent, int idx, int t) {
    BTreeNode* left = parent->children[idx];
    BTreeNode* right = parent->children[idx + 1];

    // Keo key tu parent xuong left (vi tri giua)
    left->keys[t - 1] = parent->keys[idx];

    // Copy keys tu right sang cuoi left
    for (int j = 0; j < right->num_keys; j++) {
        left->keys[j + t] = right->keys[j];
    }

    // Copy children tu right sang left (neu khong phai leaf)
    if (!left->is_leaf) {
        for (int j = 0; j <= right->num_keys; j++) {
            left->children[j + t] = right->children[j];
        }
    }

    left->num_keys += right->num_keys + 1;  // +1 cho key keo tu parent

    // Dich trai keys va children cua parent
    for (int j = idx; j < parent->num_keys - 1; j++) {
        parent->keys[j] = parent->keys[j + 1];
    }
    for (int j = idx + 1; j < parent->num_keys; j++) {
        parent->children[j] = parent->children[j + 1];
    }
    parent->num_keys--;

    // Free node right (da merge vao left)
    free(right->keys);
    free(right->children);
    free(right);
}

// fill_child - Dam bao children[idx] co >= t keys truoc khi di xuong
// Thu borrow tu sibling trai/phai, neu khong duoc thi merge
static void fill_child(BTreeNode* parent, int idx, int t) {
    // Thu borrow tu sibling trai
    if (idx > 0 && parent->children[idx - 1]->num_keys >= t) {
        BTreeNode* child = parent->children[idx];
        BTreeNode* left_sibling = parent->children[idx - 1];

        // Dich phai keys va children cua child
        for (int j = child->num_keys - 1; j >= 0; j--) {
            child->keys[j + 1] = child->keys[j];
        }
        if (!child->is_leaf) {
            for (int j = child->num_keys; j >= 0; j--) {
                child->children[j + 1] = child->children[j];
            }
        }

        // Keo key tu parent xuong child
        child->keys[0] = parent->keys[idx - 1];

        // Chuyen child cuoi cua left_sibling sang child
        if (!child->is_leaf) {
            child->children[0] = left_sibling->children[left_sibling->num_keys];
        }

        // Day key cuoi cua left_sibling len parent
        parent->keys[idx - 1] = left_sibling->keys[left_sibling->num_keys - 1];

        child->num_keys++;
        left_sibling->num_keys--;
        return;
    }

    // Thu borrow tu sibling phai
    if (idx < parent->num_keys && parent->children[idx + 1]->num_keys >= t) {
        BTreeNode* child = parent->children[idx];
        BTreeNode* right_sibling = parent->children[idx + 1];

        // Keo key tu parent xuong cuoi child
        child->keys[child->num_keys] = parent->keys[idx];

        // Chuyen child dau cua right_sibling sang child
        if (!child->is_leaf) {
            child->children[child->num_keys + 1] = right_sibling->children[0];
        }

        // Day key dau cua right_sibling len parent
        parent->keys[idx] = right_sibling->keys[0];

        // Dich trai keys va children cua right_sibling
        for (int j = 0; j < right_sibling->num_keys - 1; j++) {
            right_sibling->keys[j] = right_sibling->keys[j + 1];
        }
        if (!right_sibling->is_leaf) {
            for (int j = 0; j < right_sibling->num_keys; j++) {
                right_sibling->children[j] = right_sibling->children[j + 1];
            }
        }

        child->num_keys++;
        right_sibling->num_keys--;
        return;
    }

    // Khong borrow duoc -> merge
    if (idx < parent->num_keys) {
        merge_children(parent, idx, t);
    } else {
        merge_children(parent, idx - 1, t);
    }
}

// delete_from_node - Xoa key tu mot node (de quy)
static int delete_from_node(BTreeNode* node, const char* name, int t,
                             uint64_t* comp) {
    int idx = find_key_index(node, name);

    // Case 1: Key nam trong node hien tai
    if (idx < node->num_keys && strcmp(node->keys[idx].name, name) == 0) {
        (*comp)++;

        if (node->is_leaf) {
            // Case 1a: Node la leaf -> xoa truc tiep, dich trai
            for (int j = idx; j < node->num_keys - 1; j++) {
                node->keys[j] = node->keys[j + 1];
            }
            node->num_keys--;
            return 0;
        }

        // Case 1b: Node la internal
        if (node->children[idx]->num_keys >= t) {
            // Thay bang predecessor
            DirEntry pred = get_predecessor(node->children[idx]);
            node->keys[idx] = pred;
            return delete_from_node(node->children[idx], pred.name, t, comp);
        }

        if (node->children[idx + 1]->num_keys >= t) {
            // Thay bang successor
            DirEntry succ = get_successor(node->children[idx + 1]);
            node->keys[idx] = succ;
            return delete_from_node(node->children[idx + 1], succ.name, t, comp);
        }

        // Ca 2 con deu co t-1 keys -> merge roi xoa tu node merged
        merge_children(node, idx, t);
        return delete_from_node(node->children[idx], name, t, comp);
    }

    // Case 2: Key khong nam trong node hien tai
    (*comp)++;

    if (node->is_leaf) {
        return -1;  // Khong tim thay
    }

    // Dam bao con thich hop co du keys truoc khi di xuong
    bool should_merge = (node->children[idx]->num_keys < t);
    
    if (should_merge) {
        fill_child(node, idx, t);
    }

    // Sau khi fill/merge, idx co the da thay doi
    // Neu da merge voi con phai va idx la key cuoi thi di vao con truoc do
    if (should_merge && idx > node->num_keys) {
        return delete_from_node(node->children[idx - 1], name, t, comp);
    }

    return delete_from_node(node->children[idx], name, t, comp);
}

// ===================================================================
// Public API
// ===================================================================

// btree_create - Khoi tao B-Tree rong
BTree* btree_create(void) {
    BTree* tree = (BTree*)malloc(sizeof(BTree));
    if (!tree) {
        fprintf(stderr, "Error: Cannot allocate BTree\n");
        return NULL;
    }

    tree->min_degree = BTREE_MIN_DEGREE;
    tree->count = 0;
    tree->root = create_node(BTREE_MIN_DEGREE, true);  // Root bat dau la leaf

    if (!tree->root) {
        free(tree);
        return NULL;
    }

    return tree;
}

// btree_insert - Them entry vao B-Tree
int btree_insert(BTree* tree, const DirEntry* entry) {
    if (!tree || !entry) return -1;

    int t = tree->min_degree;

    // Truong hop dac biet: root day -> tao root moi, split root cu
    if (tree->root->num_keys == 2 * t - 1) {
        BTreeNode* new_root = create_node(t, false);
        if (!new_root) return -1;

        new_root->children[0] = tree->root;
        split_child(new_root, 0, t);

        // Xac dinh con nao de chen
        int i = 0;
        if (strcmp(entry->name, new_root->keys[0].name) > 0) {
            i = 1;
        }
        insert_nonfull(new_root->children[i], entry, t);

        tree->root = new_root;
    } else {
        insert_nonfull(tree->root, entry, t);
    }

    tree->count++;
    return 0;
}

// btree_lookup - Tim kiem entry O(log n)
DirEntry* btree_lookup(BTree* tree, const char* name, uint64_t* comparisons) {
    if (!tree || !name) return NULL;

    uint64_t comp = 0;
    DirEntry* result = search_node(tree->root, name, &comp);

    if (comparisons) *comparisons = comp;
    return result;
}

// btree_delete - Xoa entry tu B-Tree
int btree_delete(BTree* tree, const char* name, uint64_t* comparisons) {
    if (!tree || !name || !tree->root) return -1;

    uint64_t comp = 0;
    int result = delete_from_node(tree->root, name, tree->min_degree, &comp);

    if (comparisons) *comparisons = comp;

    if (result == 0) {
        tree->count--;

        // Neu root het keys va co con -> thu nho cay
        if (tree->root->num_keys == 0 && !tree->root->is_leaf) {
            BTreeNode* old_root = tree->root;
            tree->root = tree->root->children[0];
            free(old_root->keys);
            free(old_root->children);
            free(old_root);
        }
    }

    return result;
}

// btree_memory_usage - Tong bo nho cua toan bo cay
size_t btree_memory_usage(const BTree* tree) {
    if (!tree) return 0;
    return sizeof(BTree) + node_memory_usage(tree->root, tree->min_degree);
}

// btree_height - Chieu cao cay (tinh tu root = 1)
int btree_height(const BTree* tree) {
    if (!tree || !tree->root) return 0;

    int height = 1;
    BTreeNode* node = tree->root;
    while (!node->is_leaf) {
        height++;
        node = node->children[0];
    }
    return height;
}

// btree_stats - In thong ke cay (debug)
void btree_stats(const BTree* tree) {
    if (!tree) return;

    printf("  B-Tree Statistics:\n");
    printf("  ─────────────────────────────────────────────\n");
    printf("  Entries:        %d\n", tree->count);
    printf("  Min Degree (t): %d\n", tree->min_degree);
    printf("  Max keys/node:  %d\n", 2 * tree->min_degree - 1);
    printf("  Tree Height:    %d\n", btree_height(tree));
    printf("  Total Nodes:    %d\n", node_count(tree->root));
    printf("  Memory Usage:   %zu KB\n", btree_memory_usage(tree) / 1024);
}

// btree_destroy - Giai phong toan bo cay
void btree_destroy(BTree* tree) {
    if (!tree) return;
    destroy_node(tree->root);
    free(tree);
}
