// O(N!) — brute-force Travelling Salesman (all permutations)
//
// WARNING: Factorial complexity.  Only feasible for N ≲ 12.
//   ./tester --filter nfact --ns 3,4,5,6,7,8,9,10,11
#include <cstdlib>
#include <climits>

volatile int sink;

static int n;
static int* dist;  // n×n distance matrix

static int bestCost;

static void solve(int* perm, bool* used, int depth, int cost) {
    if (depth == n) {
        // Add return edge.
        int total = cost + dist[perm[depth - 1] * n + perm[0]];
        if (total < bestCost) bestCost = total;
        return;
    }
    for (int c = 0; c < n; ++c) {
        if (used[c]) continue;
        int edge = depth > 0 ? dist[perm[depth - 1] * n + c] : 0;
        perm[depth] = c;
        used[c] = true;
        solve(perm, used, depth + 1, cost + edge);
        used[c] = false;
    }
}

int main(int argc, char* argv[]) {
    n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    dist = new int[n * n];
    int v = 7;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            v = v * 1103515245 + 12345;
            dist[i * n + j] = (i == j) ? 0 : ((v >> 8) & 0xFF) + 1;
        }

    int* perm = new int[n];
    bool* used = new bool[n];
    for (int i = 0; i < n; ++i) used[i] = false;
    bestCost = INT_MAX;

    solve(perm, used, 0, 0);
    sink = bestCost;

    delete[] dist;
    delete[] perm;
    delete[] used;
    return 0;
}
