# custom-malloc

A custom implementation of `malloc`, `free`, `calloc`, and `realloc` in C. The heap grows with `sbrk()`, and free blocks are split and coalesced to reduce fragmentation.

It builds four shared libraries, one for each placement strategy:

| Library | Strategy |
| --- | --- |
| `lib/libmalloc-ff.so` | First Fit |
| `lib/libmalloc-nf.so` | Next Fit |
| `lib/libmalloc-bf.so` | Best Fit |
| `lib/libmalloc-wf.so` | Worst Fit |

## Build

Requires Linux and `gcc`.

```
mkdir -p lib
make
```

## Run

Use `LD_PRELOAD` to replace the system allocator with one of the libraries:

```
env LD_PRELOAD=lib/libmalloc-ff.so tests/ffnf
```

When the program exits, the allocator prints heap statistics: mallocs, frees, block reuses, heap grows, splits, coalesces, and max heap size.

## Tests

| Test | What it checks |
| --- | --- |
| `test1`–`test4` | Basic allocation and freeing |
| `ffnf` | First Fit vs. Next Fit placement |
| `bfwf` | Best Fit vs. Worst Fit placement |
| `calloc`, `realloc` | `calloc` and `realloc` behavior |
| `stress_test` | Small, mixed-size, fragmenting, and coalescing workloads |

`src/benchmark.c` times an allocation-heavy workload for comparing the strategies.
