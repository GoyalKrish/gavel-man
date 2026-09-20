// O(N^2) — all-pairs sum
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Double nested loop: sum all pairs (i, j).
    long long sum = 0;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            sum += (long long)(i ^ j);
    sink = sum;
    return 0;
}
