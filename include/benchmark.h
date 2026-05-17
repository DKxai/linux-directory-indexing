// benchmark.h - API chay Benchmark

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "common.h"
#include "dir_entry.h"

// MethodType - Enum xac dinh loai thuat toan
typedef enum {
    METHOD_LINEAR = 0,
    METHOD_HASH,
    METHOD_BTREE,
    METHOD_HTREE,
    METHOD_COUNT 
} MethodType;

// run_single_benchmark - Chay test cho 1 thuat toan, 1 mang cho truoc
BenchmarkResult run_single_benchmark(MethodType method, int num_entries, 
                                      DirEntry* entries);

// run_all_benchmarks - Chay test tat ca thuat toan, cho moi size
int run_all_benchmarks(BenchmarkResult* results, int max_results);

// print_results_table - Render du lieu dang table ra man hinh
void print_results_table(const BenchmarkResult* results, int count);

// export_csv - Luu array du lieu benchmark ra .csv
int export_csv(const BenchmarkResult* results, int count, const char* filename);

// method_name - Convert MethodType thanh chuoi char
const char* method_name(MethodType method);

#endif /* BENCHMARK_H */
