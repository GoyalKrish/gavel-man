// O(2^N) — naive recursive Fibonacci
//
// WARNING: This program has exponential time complexity.  It will only
// complete in reasonable time for small N (N ≲ 35).  When running the
// tester, use a small-N list:
//   ./tester --filter 2n --ns 4,8,12,16,20,24,28
#include <cstdlib>

volatile long long sink;

static long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    sink = fib(n);
    return 0;
}
