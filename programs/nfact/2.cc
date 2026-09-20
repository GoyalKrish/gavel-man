// O(N!) — enumerate all derangements (permutations with no fixed points)
//
// WARNING: Factorial complexity.  Only feasible for N ≲ 12.
//   ./tester --filter nfact --ns 3,4,5,6,7,8,9,10,11
#include <cstdlib>

volatile long long sink;

static int n;
static long long count;

static void derange(int* perm, bool* used, int depth) {
    if (depth == n) {
        ++count;
        return;
    }
    for (int c = 0; c < n; ++c) {
        if (used[c] || c == depth) continue;  // skip fixed points
        perm[depth] = c;
        used[c] = true;
        derange(perm, used, depth + 1);
        used[c] = false;
    }
}

int main(int argc, char* argv[]) {
    n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    int* perm = new int[n];
    bool* used = new bool[n];
    for (int i = 0; i < n; ++i) used[i] = false;
    count = 0;

    derange(perm, used, 0);
    sink = count;

    delete[] perm;
    delete[] used;
    return 0;
}
