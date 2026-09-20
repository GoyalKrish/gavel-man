// O(log N) — repeated halving with amplified work
//
// The pure log-N loop runs only ~12 iterations at N=4096.
// We repeat the whole log-N computation REPS times to generate
// enough instructions for the constant overhead not to dominate.
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    // Repeat the log-N work many times so total instructions ∝ log(N).
    // REPS is fixed (does not depend on N) so it only scales the constant.
    const int REPS = 5000;
    long long total = 0;

    for (int r = 0; r < REPS; ++r) {
        long long v = n;
        while (v > 0) {
            total += v;
            v >>= 1;
        }
    }
    sink = total;
    return 0;
}
