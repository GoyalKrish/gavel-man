// O(N log N) — compute N * log2(N) iterations explicitly
#include <cstdlib>
#include <cmath>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Do exactly n * floor(log2(n)) iterations of simple work.
    int logn = 0;
    { int v = n; while (v > 1) { v >>= 1; ++logn; } }
    if (logn < 1) logn = 1;

    long long sum = 0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < logn; ++j)
            sum += i ^ j;
    sink = sum;
    return 0;
}
