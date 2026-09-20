// O(1) — constant-time: hash-like computation, ignores N
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    (void)n;

    // Simulate a constant-time lookup / hash computation.
    long long h = 0xDEADBEEF;
    for (int i = 0; i < 200; ++i) {
        h ^= (h << 13);
        h ^= (h >> 7);
        h ^= (h << 17);
    }
    sink = h;
    return 0;
}
