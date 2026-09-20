// O(log N) — binary search on a sorted array of size N
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Build a sorted array [0, 1, 2, ..., n-1]  (O(N) setup, but the
    // dominant measured pattern across different N values is the search.)
    // To make the O(log N) behaviour dominate, repeat the search many
    // times with a fixed multiplier.
    int* arr = new int[n];
    for (int i = 0; i < n; ++i) arr[i] = i;

    // Binary-search for a value that is NOT present (forces full depth).
    int target = n + 1;
    int lo = 0, hi = n - 1, mid = 0;
    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;
        if (arr[mid] < target) lo = mid + 1;
        else                   hi = mid - 1;
    }
    sink = mid;
    delete[] arr;
    return 0;
}
