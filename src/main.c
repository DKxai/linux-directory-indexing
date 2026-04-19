// main.c - Application Entry Point

#include "benchmark.h"
#include "common.h"
#include "dir_entry.h"
#include "linear_search.h"

#define MAX_RESULTS (NUM_TEST_SIZES * METHOD_COUNT)

static BenchmarkResult g_results[MAX_RESULTS];
static int g_result_count = 0;

// run_interactive_demo - Hien thi command line interactive demo
static void run_interactive_demo(void) {
  print_header("INTERACTIVE DEMO — Linear Search");

  printf("  Nhập số entries để demo (khuyến nghị 10-1000): ");
  int n;
  // Kiem tra dau vao, default neu miss
  if (scanf("%d", &n) != 1 || n <= 0 || n > 100000) {
    printf("  ✗ Số không hợp lệ. Dùng mặc định: 100\n");
    n = 100;
  }

  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  printf("\n  Đang tạo %d directory entries...\n", n);
  DirEntry *entries = generate_random_entries(n);
  if (!entries) {
    printf("  ✗ Lỗi cấp phát bộ nhớ!\n");
    return;
  }

  printf("\n  Mẫu entries (5/%d):\n", n);
  for (int i = 0; i < 5 && i < n; i++) {
    print_dir_entry(&entries[i]);
  }

  printf("\n  Đang tạo Linear Search directory...\n");

  LinearDir *lin_dir = linear_create();

  TIMER_DECLARE();

  for (int i = 0; i < n; i++) {
    linear_insert(lin_dir, &entries[i]);
  }
  printf("  ✓ Đã insert %d entries vào Linear Search directory\n\n", n);

  const char *search_name = entries[n / 2].name;
  printf("  Demo lookup: \"%s\"\n", search_name);
  printf("  ─────────────────────────────────────────────\n");

  uint64_t comp;

  TIMER_START();
  comp = 0;
  DirEntry *found = linear_lookup(lin_dir, search_name, &comp);
  TIMER_END();
  printf("  Linear Search:  %s | %lu comparisons | %lu ns\n",
         found ? "FOUND" : "NOT FOUND", (unsigned long)comp,
         (unsigned long)TIMER_ELAPSED_NS());

  printf("\n  Memory Usage:\n");
  printf("  ─────────────────────────────────────────────\n");
  printf("  Linear Search:  %zu KB\n", linear_memory_usage(lin_dir) / 1024);

  linear_destroy(lin_dir);
  free(entries);

  printf("\n  ✓ Demo hoàn tất!\n");
}

// run_single_method - Test input so luong chi chay cho method 1
static void run_single_method(void) {
  print_header("SINGLE METHOD BENCHMARK");

  printf("  Hiện tại chỉ có Linear Search.\n");

  printf("  Nhập số entries (100-1000000): ");
  int n;
  int c;
  if (scanf("%d", &n) != 1 || n <= 0 || n > 1000000) {
    printf("  ✗ Số không hợp lệ!\n");
    while ((c = getchar()) != '\n' && c != EOF)
      ;
    return;
  }
  while ((c = getchar()) != '\n' && c != EOF)
    ;

  printf("\n  Đang chạy benchmark Linear Search với N=%d...\n", n);

  DirEntry *entries = generate_random_entries(n);
  if (!entries) {
    printf("  ✗ Lỗi cấp phát bộ nhớ!\n");
    return;
  }

  BenchmarkResult result = run_single_benchmark(METHOD_LINEAR, n, entries);
  free(entries);

  // In format report
  printf("\n  ═══ Kết quả ═══\n");
  printf("  Phương pháp:        %s\n", result.method_name);
  printf("  Số entries:         %d\n", result.num_entries);
  printf("  Insert time:        %lu μs\n",
         (unsigned long)(result.insert_time_ns / 1000));
  printf("  Avg lookup (hit):   %lu ns\n",
         (unsigned long)result.avg_lookup_hit_ns);
  printf("  Avg lookup (miss):  %lu ns\n",
         (unsigned long)result.avg_lookup_miss_ns);
  printf("  Avg comparisons:    %lu (hit), %lu (miss)\n",
         (unsigned long)result.avg_comparisons_hit,
         (unsigned long)result.avg_comparisons_miss);
  printf("  Memory usage:       %lu KB\n",
         (unsigned long)(result.memory_usage_bytes / 1024));
}

// print_menu - Render Option
static void print_menu(void) {
  printf("\n");
  printf("╔══════════════════════════════════════════════════╗\n");
  printf("║   Directory Lookup Performance Benchmark         ║\n");
  printf("║   Week 3: Linear Search Baseline                 ║\n");
  printf("║   ─────────────────────────────────────────      ║\n");
  printf("║   Operating Systems Project                      ║\n");
  printf("╠══════════════════════════════════════════════════╣\n");
  printf("║                                                  ║\n");
  printf("║   1. 🚀 Run full benchmark (Linear Search)       ║\n");
  printf("║   2. 🔬 Run single benchmark                     ║\n");
  printf("║   3. 🎮 Interactive demo                         ║\n");
  printf("║   4. 📊 View results summary                     ║\n");
  printf("║   5. 💾 Export results to CSV                     ║\n");
  printf("║   0. 🚪 Exit                                     ║\n");
  printf("║                                                  ║\n");
  printf("╚══════════════════════════════════════════════════╝\n");
  printf("  Your choice: ");
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
      printf("  ✗ Input không hợp lệ!\n");
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
        printf("\n  ⚠ Chưa có kết quả!\n");
      } else {
        print_results_table(g_results, g_result_count);
      }
      break;
    case 5:
      if (g_result_count == 0) {
        printf("\n  ⚠ Chưa có kết quả!\n");
      } else {
        export_csv(g_results, g_result_count, "results/benchmark_results.csv");
      }
      break;
    case 0:
      printf("\n  Goodbye! 👋\n\n");
      running = 0;
      break;
    default:
      printf("\n  ✗ Lựa chọn không hợp lệ!\n");
      break;
    }
  }

  return 0;
}
