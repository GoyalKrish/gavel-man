// O(N) — linear: find maximum in an unsorted array
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Build an array with a pseudo-random pattern, then linear-scan for max.
    int* arr = new int[n];
    int v = 7;
    for (int i = 0; i < n; ++i) {
        v = v * 1103515245 + 12345;
        arr[i] = v;
    }

    int mx = arr[0];
    for (int i = 1; i < n; ++i)
        if (arr[i] > mx) mx = arr[i];

    sink = mx;
    delete[] arr;
    return 0;
}
