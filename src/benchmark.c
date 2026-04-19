// benchmark.c - Thuc thi viec do hieu nang cua cac thuat toan

#include "benchmark.h"
#include "linear_search.h"

// Function: method_name - Chuyen enum method thanh ten text
const char *method_name(MethodType method) {
  switch (method) {
  case METHOD_LINEAR:
    return "Linear Search";
  default:
    return "Unknown";
  }
}

// Function: benchmark_linear - Do lookup/insert performance cua array list
// (O(n))
static BenchmarkResult benchmark_linear(int num_entries, DirEntry *entries) {
  BenchmarkResult result;
  memset(&result, 0, sizeof(result));
  strncpy(result.method_name, "Linear Search", MAX_METHOD_NAME - 1);
  result.num_entries = num_entries;

  TIMER_DECLARE();

  TIMER_START();
  LinearDir *dir = linear_create();
  for (int i = 0; i < num_entries; i++) {
    linear_insert(dir, &entries[i]);
  }
  TIMER_END();
  result.insert_time_ns = TIMER_ELAPSED_NS();

  char **hits = generate_lookup_hits(entries, num_entries, NUM_LOOKUPS_HIT);
  uint64_t total_comp_hit = 0;

  TIMER_START();
  for (int i = 0; i < NUM_LOOKUPS_HIT; i++) {
    uint64_t comp = 0;
    linear_lookup(dir, hits[i], &comp);
    total_comp_hit += comp;
  }
  TIMER_END();
  result.avg_lookup_hit_ns = TIMER_ELAPSED_NS() / NUM_LOOKUPS_HIT;
  result.avg_comparisons_hit = total_comp_hit / NUM_LOOKUPS_HIT;

  char **misses = generate_lookup_misses(NUM_LOOKUPS_MISS);
  uint64_t total_comp_miss = 0;

  TIMER_START();
  for (int i = 0; i < NUM_LOOKUPS_MISS; i++) {
    uint64_t comp = 0;
    linear_lookup(dir, misses[i], &comp);
    total_comp_miss += comp;
  }
  TIMER_END();
  result.avg_lookup_miss_ns = TIMER_ELAPSED_NS() / NUM_LOOKUPS_MISS;
  result.avg_comparisons_miss = total_comp_miss / NUM_LOOKUPS_MISS;

  result.memory_usage_bytes = linear_memory_usage(dir);

  free_lookup_targets(hits, NUM_LOOKUPS_HIT);
  free_lookup_targets(misses, NUM_LOOKUPS_MISS);
  linear_destroy(dir);

  return result;
}

// Function: run_single_benchmark - Router chay dung thuat toan theo method dinh
// danh
BenchmarkResult run_single_benchmark(MethodType method, int num_entries,
                                     DirEntry *entries) {
  switch (method) {
  case METHOD_LINEAR:
    return benchmark_linear(num_entries, entries);
  default: {
    BenchmarkResult empty;
    memset(&empty, 0, sizeof(empty));
    return empty;
  }
  }
}

// Function: run_all_benchmarks - Loop quet toan bo param, kich hoat benchmark
int run_all_benchmarks(BenchmarkResult *results, int max_results) {
  int count = 0;

  printf("\n");
  print_header("RUNNING FULL BENCHMARK");
  printf("  Methods: Linear Search\n");
  printf("  Sizes:   ");
  for (int i = 0; i < NUM_TEST_SIZES; i++) {
    printf("%d", TEST_SIZES[i]);
    if (i < NUM_TEST_SIZES - 1)
      printf(", ");
  }
  printf("\n");
  printf("  Lookups: %d hits + %d misses per test\n", NUM_LOOKUPS_HIT,
         NUM_LOOKUPS_MISS);
  print_separator();

  for (int s = 0; s < NUM_TEST_SIZES && count + METHOD_COUNT <= max_results;
       s++) {
    int n = TEST_SIZES[s];

    printf("\n▶ Testing with N = %d entries...\n", n);

    DirEntry *entries = generate_random_entries(n);
    if (!entries) {
      fprintf(stderr, "Error: Cannot generate %d entries\n", n);
      continue;
    }

    for (int m = 0; m < METHOD_COUNT; m++) {
      printf("  ├─ %-15s ... ", method_name((MethodType)m));
      fflush(stdout);

      results[count] = run_single_benchmark((MethodType)m, n, entries);

      printf("done (lookup: %lu ns, %lu comparisons)\n",
             (unsigned long)results[count].avg_lookup_hit_ns,
             (unsigned long)results[count].avg_comparisons_hit);
      count++;
    }

    free(entries);
    printf("  └─ Complete!\n");
  }

  printf("\n");
  print_header("BENCHMARK COMPLETE");
  printf("  Total tests: %d\n", count);

  return count;
}

// Function: print_results_table - Render giao dien
void print_results_table(const BenchmarkResult *results, int count) {
  print_header("BENCHMARK RESULTS — Linear Search Baseline");

  printf("%-15s %8s %12s %12s %12s %10s %10s %12s\n", "Method", "N",
         "Insert(μs)", "Lookup(μs)", "Miss(μs)", "Comp(hit)", "Comp(miss)",
         "Memory(KB)");
  printf("─────────────── ──────── ──────────── ──────────── "
         "──────────── ────────── ────────── ────────────\n");

  for (int i = 0; i < count; i++) {
    const BenchmarkResult *r = &results[i];
    printf("%-15s %8d %12lu %12lu %12lu %10lu %10lu %12lu\n", r->method_name,
           r->num_entries, (unsigned long)(r->insert_time_ns / 1000),
           (unsigned long)r->avg_lookup_hit_ns,
           (unsigned long)r->avg_lookup_miss_ns,
           (unsigned long)r->avg_comparisons_hit,
           (unsigned long)r->avg_comparisons_miss,
           (unsigned long)(r->memory_usage_bytes / 1024));
  }
}

// Function: export_csv - Ghi header/data table xuong csv format
int export_csv(const BenchmarkResult *results, int count,
               const char *filename) {
  FILE *fp = fopen(filename, "w");
  if (!fp) {
    fprintf(stderr, "Error: Cannot open %s for writing\n", filename);
    return -1;
  }

  fprintf(fp, "method,num_entries,insert_time_ns,avg_lookup_hit_ns,"
              "avg_lookup_miss_ns,avg_comparisons_hit,avg_comparisons_miss,"
              "memory_usage_bytes\n");

  for (int i = 0; i < count; i++) {
    const BenchmarkResult *r = &results[i];
    fprintf(fp, "%s,%d,%lu,%lu,%lu,%lu,%lu,%lu\n", r->method_name,
            r->num_entries, (unsigned long)r->insert_time_ns,
            (unsigned long)r->avg_lookup_hit_ns,
            (unsigned long)r->avg_lookup_miss_ns,
            (unsigned long)r->avg_comparisons_hit,
            (unsigned long)r->avg_comparisons_miss,
            (unsigned long)r->memory_usage_bytes);
  }

  fclose(fp);
  printf("  ✓ Results exported to: %s\n", filename);
  return 0;
}
