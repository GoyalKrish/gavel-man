// O(N^2) — selection sort
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    int* arr = new int[n];
    int v = 13;
    for (int i = 0; i < n; ++i) {
        v = v * 1103515245 + 12345;
        arr[i] = v;
    }

    // Selection sort: for each position, scan the rest for the minimum.
    for (int i = 0; i < n - 1; ++i) {
        int mi = i;
        for (int j = i + 1; j < n; ++j)
            if (arr[j] < arr[mi]) mi = j;
        int tmp = arr[i]; arr[i] = arr[mi]; arr[mi] = tmp;
    }

    sink = arr[0];
    delete[] arr;
    return 0;
}
