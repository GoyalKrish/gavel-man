// O(N) — linear scan with heavier per-element work
//
// The simple sum loop had too few instructions per iteration,
// making the constant overhead dominate at small N.
// This version does ~10 operations per element.
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    long long sum = 0;
    long long a = 1, b = 1;
    for (int i = 0; i < n; ++i) {
        // Do several operations per iteration to boost the linear signal.
        long long c = a + b;
        a = b;
        b = c;
        sum += c ^ (c >> 3);
        sum += (c * 31 + 17) & 0xFFFF;
    }
    sink = sum;
    return 0;
}
