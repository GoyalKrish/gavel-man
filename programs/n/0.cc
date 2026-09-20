// O(N) — linear scan: sum of first N integers
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    long long sum = 0;
    for (int i = 0; i < n; ++i)
        sum += i;
    sink = sum;
    return 0;
}
