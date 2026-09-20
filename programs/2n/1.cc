// O(2^N) — enumerate all subsets of {0, ..., N-1}
//
// WARNING: Exponential complexity.  Only feasible for N ≲ 25.
//   ./tester --filter 2n --ns 4,8,10,12,14,16,18,20
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Iterate over all 2^N subsets using bitmask enumeration.
    // For each subset compute its XOR sum.
    long long total = 0;
    long long limit = 1LL << n;   // 2^N
    for (long long mask = 0; mask < limit; ++mask) {
        int xsum = 0;
        for (int bit = 0; bit < n; ++bit)
            if (mask & (1LL << bit))
                xsum ^= bit;
        total += xsum;
    }
    sink = total;
    return 0;
}
