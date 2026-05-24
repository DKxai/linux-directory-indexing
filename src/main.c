// main.c - Application Entry Point

#include "benchmark.h"
#include "common.h"
#include "dir_entry.h"
#include "linear_search.h"
#include "hash_table.h"
#include "btree.h"
#include "htree.h"

#define MAX_RESULTS (NUM_TEST_SIZES * METHOD_COUNT)

static BenchmarkResult g_results[MAX_RESULTS];
static int g_result_count = 0;

// run_interactive_demo - Hien thi command line interactive demo
static void run_interactive_demo(void) {
  print_header("INTERACTIVE DEMO — Linear vs Hash vs B-Tree vs HTree");

  printf("  Nhap so entries de demo (khuyen nghi 10-1000): ");
  int n;
  // Kiem tra dau vao, default neu miss
  if (scanf("%d", &n) != 1 || n <= 0 || n > 100000) {
    printf("  ✗ So khong hop le. Dung mac dinh: 100\n");
    n = 100;
  }

  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  printf("\n  Dang tao %d directory entries...\n", n);
  DirEntry *entries = generate_random_entries(n);
  if (!entries) {
    printf("  ✗ Loi cap phat bo nho!\n");
    return;
  }

  printf("\n  Mau entries (5/%d):\n", n);
  for (int i = 0; i < 5 && i < n; i++) {
    print_dir_entry(&entries[i]);
  }

  // === TAO CA 4 CAU TRUC ===
  printf("\n  Dang tao Linear Search directory...\n");
  LinearDir *lin_dir = linear_create();
  printf("  Dang tao Hash Table...\n");
  HashTable *ht = hash_create();
  printf("  Dang tao B-Tree...\n");
  BTree *bt = btree_create();
  printf("  Dang tao HTree (ext4)...\n");
  HTree *htree = htree_create();

  TIMER_DECLARE();

  // Insert vao Linear
  for (int i = 0; i < n; i++) {
    linear_insert(lin_dir, &entries[i]);
  }
  printf("  ✓ Da insert %d entries vao Linear Search directory\n", n);

  // Insert vao Hash Table
  for (int i = 0; i < n; i++) {
    hash_insert(ht, &entries[i]);
  }
  printf("  ✓ Da insert %d entries vao Hash Table\n", n);

  // Insert vao B-Tree
  for (int i = 0; i < n; i++) {
    btree_insert(bt, &entries[i]);
  }
  printf("  ✓ Da insert %d entries vao B-Tree\n", n);

  // Insert vao HTree
  for (int i = 0; i < n; i++) {
    htree_insert(htree, &entries[i]);
  }
  printf("  ✓ Da insert %d entries vao HTree\n\n", n);

  // --- Demo LOOKUP ---
  const char *search_name = entries[n / 2].name;
  printf("  Demo lookup: \"%s\"\n", search_name);
  printf("  ─────────────────────────────────────────────\n");

  uint64_t comp;

  // Lookup Linear
  TIMER_START();
  comp = 0;
  DirEntry *found_lin = linear_lookup(lin_dir, search_name, &comp);
  TIMER_END();
  printf("  Linear Search:  %s | %lu comparisons | %lu ns\n",
         found_lin ? "FOUND" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Lookup Hash
  TIMER_START();
  comp = 0;
  DirEntry *found_hash = hash_lookup(ht, search_name, &comp);
  TIMER_END();
  printf("  Hash Table:     %s | %lu comparisons | %lu ns\n",
         found_hash ? "FOUND" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Lookup B-Tree
  TIMER_START();
  comp = 0;
  DirEntry *found_bt = btree_lookup(bt, search_name, &comp);
  TIMER_END();
  printf("  B-Tree:         %s | %lu comparisons | %lu ns\n",
         found_bt ? "FOUND" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Lookup HTree
  TIMER_START();
  comp = 0;
  DirEntry *found_ht = htree_lookup(htree, search_name, &comp);
  TIMER_END();
  printf("  HTree:          %s | %lu comparisons | %lu ns\n",
         found_ht ? "FOUND" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // --- Demo DELETE ---
  const char *delete_name = entries[n / 4].name;
  printf("\n  Demo delete: \"%s\"\n", delete_name);
  printf("  ─────────────────────────────────────────────\n");

  // Delete Linear
  TIMER_START();
  comp = 0;
  int del_result_lin = linear_delete(lin_dir, delete_name, &comp);
  TIMER_END();
  printf("  Linear Delete:  %s | %lu comparisons | %lu ns\n",
         del_result_lin == 0 ? "DELETED" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Delete Hash
  TIMER_START();
  comp = 0;
  int del_result_hash = hash_delete(ht, delete_name, &comp);
  TIMER_END();
  printf("  Hash Delete:    %s | %lu comparisons | %lu ns\n",
         del_result_hash == 0 ? "DELETED" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Delete B-Tree
  TIMER_START();
  comp = 0;
  int del_result_bt = btree_delete(bt, delete_name, &comp);
  TIMER_END();
  printf("  B-Tree Delete:  %s | %lu comparisons | %lu ns\n",
         del_result_bt == 0 ? "DELETED" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Delete HTree
  TIMER_START();
  comp = 0;
  int del_result_htree = htree_delete(htree, delete_name, &comp);
  TIMER_END();
  printf("  HTree Delete:   %s | %lu comparisons | %lu ns\n",
         del_result_htree == 0 ? "DELETED" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  // Verify delete
  comp = 0;
  DirEntry *verify_lin = linear_lookup(lin_dir, delete_name, &comp);
  printf("\n  Verify Linear:  %s (sau khi delete)\n",
         verify_lin ? "FOUND (LOI!)" : "NOT FOUND (dung)");

  comp = 0;
  DirEntry *verify_hash = hash_lookup(ht, delete_name, &comp);
  printf("  Verify Hash:    %s (sau khi delete)\n",
         verify_hash ? "FOUND (LOI!)" : "NOT FOUND (dung)");

  comp = 0;
  DirEntry *verify_bt = btree_lookup(bt, delete_name, &comp);
  printf("  Verify B-Tree:  %s (sau khi delete)\n",
         verify_bt ? "FOUND (LOI!)" : "NOT FOUND (dung)");

  comp = 0;
  DirEntry *verify_ht = htree_lookup(htree, delete_name, &comp);
  printf("  Verify HTree:   %s (sau khi delete)\n",
         verify_ht ? "FOUND (LOI!)" : "NOT FOUND (dung)");

  printf("\n  Memory Usage:\n");
  printf("  ─────────────────────────────────────────────\n");
  printf("  Linear Search:  %zu KB\n", linear_memory_usage(lin_dir) / 1024);
  printf("  Hash Table:     %zu KB\n", hash_memory_usage(ht) / 1024);
  printf("  B-Tree:         %zu KB\n", btree_memory_usage(bt) / 1024);
  printf("  HTree:          %zu KB\n", htree_memory_usage(htree) / 1024);
  printf("  Entries count:  Linear=%d, Hash=%d, B-Tree=%d, HTree=%d (da xoa 1 moi ben)\n",
         lin_dir->count, ht->count, bt->count, htree->count);

  // In stats
  printf("\n");
  hash_stats(ht);
  printf("\n");
  btree_stats(bt);
  printf("\n");
  htree_stats(htree);

  linear_destroy(lin_dir);
  hash_destroy(ht);
  btree_destroy(bt);
  htree_destroy(htree);
  free(entries);

  printf("\n  ✓ Demo hoan tat!\n");
}

// run_single_method - Test input so luong cho 1 method duoc chon
static void run_single_method(void) {
  print_header("SINGLE METHOD BENCHMARK");

  printf("  Chon phuong phap:\n");
  printf("    1. Linear Search\n");
  printf("    2. Hash Table\n");
  printf("    3. B-Tree\n");
  printf("    4. HTree (ext4)\n");
  printf("  Lua chon: ");

  int method_choice;
  int c;
  if (scanf("%d", &method_choice) != 1 || method_choice < 1 || method_choice > 4) {
    printf("  ✗ Lua chon khong hop le!\n");
    while ((c = getchar()) != '\n' && c != EOF)
      ;
    return;
  }
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  MethodType selected;
  if (method_choice == 1) selected = METHOD_LINEAR;
  else if (method_choice == 2) selected = METHOD_HASH;
  else if (method_choice == 3) selected = METHOD_BTREE;
  else selected = METHOD_HTREE;

  printf("  Nhap so entries (100-1000000): ");
  int n;
  if (scanf("%d", &n) != 1 || n <= 0 || n > 1000000) {
    printf("  ✗ So khong hop le!\n");
    while ((c = getchar()) != '\n' && c != EOF)
      ;
    return;
  }
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  printf("\n  Dang chay benchmark %s voi N=%d...\n", method_name(selected), n);

  DirEntry *entries = generate_random_entries(n);
  if (!entries) {
    printf("  ✗ Loi cap phat bo nho!\n");
    return;
  }

  BenchmarkResult result = run_single_benchmark(selected, n, entries);
  free(entries);

  // In format report
  printf("\n  ═══ Ket qua ═══\n");
  printf("  Phuong phap:        %s\n", result.method_name);
  printf("  So entries:         %d\n", result.num_entries);
  printf("  Insert time:        %lu us\n",
         (unsigned long)(result.insert_time_ns / 1000));
  printf("  Avg lookup (hit):   %lu ns\n",
         (unsigned long)result.avg_lookup_hit_ns);
  printf("  Avg lookup (miss):  %lu ns\n",
         (unsigned long)result.avg_lookup_miss_ns);
  printf("  Avg comparisons:    %lu (hit), %lu (miss)\n",
         (unsigned long)result.avg_comparisons_hit,
         (unsigned long)result.avg_comparisons_miss);
  printf("  Avg delete time:    %lu ns\n",
         (unsigned long)result.avg_delete_time_ns);
  printf("  Avg delete comp:    %lu\n",
         (unsigned long)result.avg_delete_comparisons);
  printf("  Memory usage:       %lu KB\n",
         (unsigned long)(result.memory_usage_bytes / 1024));
}

// ANSI color codes
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_DIM     "\033[2m"
#define C_CYAN    "\033[36m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_RED     "\033[31m"
#define C_BLUE    "\033[34m"
#define C_MAGENTA "\033[35m"
#define C_WHITE   "\033[97m"
#define C_BG_DARK "\033[48;5;235m"
#define C_ORANGE  "\033[38;5;208m"
#define C_GRAY    "\033[38;5;245m"

// print_menu - Render Option
static void print_menu(void) {
  printf("\n");
  printf(C_CYAN C_BOLD);
  printf("  ┌──────────────────────────────────────────────────────────┐\n");
  printf("  │                                                          │\n");
  printf("  │   ██████╗ ██╗██████╗     ██╗      ██████╗  ██████╗ ██╗  │\n");
  printf("  │   ██╔══██╗██║██╔══██╗    ██║     ██╔═══██╗██╔═══██╗██║  │\n");
  printf("  │   ██║  ██║██║██████╔╝    ██║     ██║   ██║██║   ██║██║  │\n");
  printf("  │   ██║  ██║██║██╔══██╗    ██║     ██║   ██║██║   ██║██║  │\n");
  printf("  │   ██████╔╝██║██║  ██║    ███████╗╚██████╔╝╚██████╔╝██║  │\n");
  printf("  │   ╚═════╝ ╚═╝╚═╝  ╚═╝    ╚══════╝ ╚═════╝  ╚═════╝ ╚═╝  │\n");
  printf("  │                                                          │\n");
  printf(C_RESET);
  printf(C_WHITE "  │" C_RESET "   " C_BOLD "Directory Lookup Performance Benchmark" C_RESET "          " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_DIM "Week 7  •  Operating Systems Project" C_RESET "            " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_DIM "Linear • Hash • B-Tree • HTree (ext4)" C_RESET "          " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │                                                          │" C_RESET "\n");
  printf(C_CYAN "  ├──────────────────────────────────────────────────────────┤" C_RESET "\n");
  printf(C_WHITE "  │                                                          │" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_GREEN C_BOLD "[1]" C_RESET " 🚀 Run full benchmark " C_DIM "(all 4 methods)" C_RESET "        " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_BLUE C_BOLD "[2]" C_RESET " 🔬 Run single method benchmark" C_RESET "               " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_MAGENTA C_BOLD "[3]" C_RESET " 🎮 Interactive demo " C_DIM "(compare live)" C_RESET "         " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_ORANGE C_BOLD "[4]" C_RESET " 📊 View results summary" C_RESET "                     " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_YELLOW C_BOLD "[5]" C_RESET " 💾 Export results to CSV" C_RESET "                    " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │" C_RESET "   " C_RED C_BOLD "[0]" C_RESET " 🚪 Exit" C_RESET "                                     " C_WHITE "│" C_RESET "\n");
  printf(C_WHITE "  │                                                          │" C_RESET "\n");
  printf(C_CYAN "  └──────────────────────────────────────────────────────────┘" C_RESET "\n");
  printf("\n  " C_CYAN C_BOLD "▸" C_RESET " Your choice: ");
}

// main - Router cac dieu huong menu va parse c-arguments
int main(int argc, char *argv[]) {
  // Check flags skip menu
  if (argc > 1) {
    if (strcmp(argv[1], "--full") == 0 || strcmp(argv[1], "-f") == 0) {
      g_result_count = run_all_benchmarks(g_results, MAX_RESULTS);
      print_results_table(g_results, g_result_count);
      export_csv(g_results, g_result_count, "results/benchmark_results.csv");
      return 0;
    }
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
      printf("Usage: %s [options]\n", argv[0]);
      printf("Options:\n");
      printf("  --full, -f     Run full benchmark (no menu)\n");
      printf("  --help, -h     Show this help\n");
      printf("  (no args)      Interactive menu mode\n");
      return 0;
    }
  }

  int running = 1;
  // Main UI Loop
  while (running) {
    print_menu();

    int choice;
    if (scanf("%d", &choice) != 1) {
      printf("  ✗ Input khong hop le!\n");
      int c;
      while ((c = getchar()) != '\n' && c != EOF)
        ;
      continue;
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
      ;

    switch (choice) {
    case 1:
      g_result_count = run_all_benchmarks(g_results, MAX_RESULTS);
      if (g_result_count > 0) {
        print_results_table(g_results, g_result_count);
        export_csv(g_results, g_result_count, "results/benchmark_results.csv");
      }
      break;
    case 2:
      run_single_method();
      break;
    case 3:
      run_interactive_demo();
      break;
    case 4:
      if (g_result_count == 0) {
        printf("\n  ⚠ Chua co ket qua!\n");
      } else {
        print_results_table(g_results, g_result_count);
      }
      break;
    case 5:
      if (g_result_count == 0) {
        printf("\n  ⚠ Chua co ket qua!\n");
      } else {
        export_csv(g_results, g_result_count, "results/benchmark_results.csv");
      }
      break;
    case 0:
      printf("\n  Goodbye! 👋\n\n");
      running = 0;
      break;
    default:
      printf("\n  ✗ Lua chon khong hop le!\n");
      break;
    }
  }

  return 0;
}
