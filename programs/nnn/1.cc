// O(N^3) — triple nested loop: all triples sum
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Count all ordered triples (i, j, k) with 0 <= i < j < k < n.
    // Number of iterations = C(n,3) = O(N^3).
    long long count = 0;
    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j)
            for (int k = j + 1; k < n; ++k)
                ++count;
    sink = count;
    return 0;
}
