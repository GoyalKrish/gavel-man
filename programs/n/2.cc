// O(N) — linear: prefix sum computation
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Allocate, fill, and compute prefix sums — all O(N).
    long long* arr = new long long[n];
    for (int i = 0; i < n; ++i)
        arr[i] = i + 1;
    for (int i = 1; i < n; ++i)
        arr[i] += arr[i - 1];

    sink = arr[n - 1];
    delete[] arr;
    return 0;
}
