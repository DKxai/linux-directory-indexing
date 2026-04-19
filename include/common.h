/**
 * common.h - Shared types, constants, and utility macros
 * 
 * Cung cap cac kieu du lieu chung, hang so, va macro do thoi gian
 * cho toan bo du an Directory Lookup Performance Benchmark.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/resource.h>

/* ===== Constants ===== */
#define MAX_FILENAME_LEN  255
#define BLOCK_SIZE        4096
#define MAX_METHOD_NAME   32

/* Benchmark test sizes */
#define NUM_TEST_SIZES    9
static const int TEST_SIZES[NUM_TEST_SIZES] = {
    100, 500, 1000, 5000, 10000, 50000, 100000, 500000, 1000000
};

/* Number of lookups per benchmark run */
#define NUM_LOOKUPS_HIT   1000   /* Lookups for existing entries */
#define NUM_LOOKUPS_MISS  100    /* Lookups for non-existing entries */
#define NUM_DELETES       500    /* Deletes per benchmark run */
#define BENCHMARK_ROUNDS  3      /* Number of rounds to average */

/* ===== Timer Macros ===== */
/* Su dung clock_gettime(CLOCK_MONOTONIC) de do thoi gian chinh xac (nanoseconds) */
#define TIMER_DECLARE()  struct timespec _ts_start, _ts_end
#define TIMER_START()    clock_gettime(CLOCK_MONOTONIC, &_ts_start)
#define TIMER_END()      clock_gettime(CLOCK_MONOTONIC, &_ts_end)
#define TIMER_ELAPSED_NS() \
    ((uint64_t)(_ts_end.tv_sec - _ts_start.tv_sec) * 1000000000ULL + \
     (uint64_t)(_ts_end.tv_nsec - _ts_start.tv_nsec))
#define TIMER_ELAPSED_US() (TIMER_ELAPSED_NS() / 1000ULL)
#define TIMER_ELAPSED_MS() (TIMER_ELAPSED_NS() / 1000000ULL)

/* ===== Benchmark Result ===== */
typedef struct {
    char     method_name[MAX_METHOD_NAME]; /* Ten phuong phap */
    int      num_entries;                  /* So directory entries */
    uint64_t insert_time_ns;               /* Thoi gian insert tat ca entries (ns) */
    uint64_t avg_lookup_hit_ns;            /* Thoi gian trung binh lookup hit (ns) */
    uint64_t avg_lookup_miss_ns;           /* Thoi gian trung binh lookup miss (ns) */
    uint64_t avg_comparisons_hit;          /* So comparisons trung binh (hit) */
    uint64_t avg_comparisons_miss;         /* So comparisons trung binh (miss) */
    uint64_t avg_delete_time_ns;           /* Thoi gian trung binh delete (ns) */
    uint64_t avg_delete_comparisons;       /* So comparisons trung binh khi delete */
    size_t   memory_usage_bytes;           /* Bo nho su dung (bytes) */
} BenchmarkResult;

/* ===== Memory Tracking ===== */
static inline size_t get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return (size_t)usage.ru_maxrss;  /* in KB on Linux */
}

/* ===== Utility ===== */
static inline void print_separator(void) {
    printf("════════════════════════════════════════════════════════════════\n");
}

static inline void print_header(const char* title) {
    printf("\n");
    print_separator();
    printf("  %s\n", title);
    print_separator();
}

#endif /* COMMON_H */
