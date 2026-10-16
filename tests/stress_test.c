#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * Stress test suite for custom malloc allocator.
 *
 * Test 1: Rapid small allocations    — many small blocks, free all
 * Test 2: Mixed sizes                — varied sizes to trigger splits
 * Test 3: Fragmentation pattern      — alloc many, free every other, realloc
 * Test 4: Coalesce stress            — sequential free to trigger merging
 * Test 5: Realistic workload         — simulates string/buffer operations
 */

/* ── Test 1: Rapid small allocations ─────────────────────── */
void test_rapid_small(void)
{
    const int N = 2048;
    char *ptrs[N];
    int i;

    for (i = 0; i < N; i++)
        ptrs[i] = (char *)malloc(64);

    /* touch memory to make it realistic */
    for (i = 0; i < N; i++)
        memset(ptrs[i], 'A', 64);

    for (i = 0; i < N; i++)
        free(ptrs[i]);
}

/* ── Test 2: Mixed sizes (triggers splitting) ────────────── */
void test_mixed_sizes(void)
{
    const int N = 512;
    char *ptrs[N];
    size_t sizes[] = {32, 128, 512, 2048, 64, 256, 1024, 4096};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);
    int i;

    for (i = 0; i < N; i++)
    {
        size_t sz = sizes[i % nsizes];
        ptrs[i] = (char *)malloc(sz);
        memset(ptrs[i], 'B', sz);
    }

    /* free in reverse to create large contiguous free regions */
    for (i = N - 1; i >= 0; i--)
        free(ptrs[i]);
}

/* ── Test 3: Fragmentation pattern ───────────────────────── */
void test_fragmentation(void)
{
    const int N = 1024;
    char *ptrs[N];
    int i;

    /* allocate all */
    for (i = 0; i < N; i++)
        ptrs[i] = (char *)malloc(128);

    /* free every other block — creates checkerboard holes */
    for (i = 0; i < N; i += 2)
        free(ptrs[i]);

    /* now request larger blocks — tests if allocator can find/split */
    char *big[256];
    for (i = 0; i < 256; i++)
        big[i] = (char *)malloc(64);

    /* cleanup */
    for (i = 1; i < N; i += 2)
        free(ptrs[i]);
    for (i = 0; i < 256; i++)
        free(big[i]);
}

/* ── Test 4: Coalesce stress ─────────────────────────────── */
void test_coalesce(void)
{
    const int N = 512;
    char *ptrs[N];
    int i;

    /* allocate many adjacent blocks */
    for (i = 0; i < N; i++)
        ptrs[i] = (char *)malloc(256);

    /* free sequentially — should trigger chain coalescing */
    for (i = 0; i < N; i++)
        free(ptrs[i]);

    /* now a single large alloc should reuse the coalesced space */
    char *big = (char *)malloc(N * 128);
    if (big)
    {
        memset(big, 'D', N * 128);
        free(big);
    }
}

/* ── Test 5: Realistic workload (string-like operations) ── */
void test_realistic(void)
{
    const int ITERS = 500;
    int i;

    for (i = 0; i < ITERS; i++)
    {
        /* simulate reading a line */
        size_t len = 40 + (i % 200);
        char *line = (char *)malloc(len);
        memset(line, 'x', len - 1);
        line[len - 1] = '\0';

        /* simulate a copy */
        char *copy = (char *)malloc(len);
        memcpy(copy, line, len);

        /* simulate resize */
        char *bigger = (char *)realloc(copy, len * 2);
        if (bigger)
            free(bigger);

        free(line);
    }
}

int main(void)
{
    struct timespec start, end;
    double elapsed;

    clock_gettime(CLOCK_MONOTONIC, &start);

    printf("=== Stress Test Suite ===\n\n");

    printf("[1/5] Rapid small allocations (2048 x 64B)...\n");
    test_rapid_small();

    printf("[2/5] Mixed sizes (512 allocs, 8 different sizes)...\n");
    test_mixed_sizes();

    printf("[3/5] Fragmentation pattern (checkerboard free)...\n");
    test_fragmentation();

    printf("[4/5] Coalesce stress (512 sequential frees)...\n");
    test_coalesce();

    printf("[5/5] Realistic workload (500 string operations)...\n");
    test_realistic();

    clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed = (end.tv_sec - start.tv_sec) +
              (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("\n>>> Total execution time: %.6f seconds\n", elapsed);

    return 0;
}

