// O(N log N) — heap sort
#include <cstdlib>

volatile int sink;

static void siftDown(int* arr, int n, int i) {
    while (true) {
        int largest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        if (left < n && arr[left] > arr[largest])   largest = left;
        if (right < n && arr[right] > arr[largest]) largest = right;
        if (largest == i) break;
        int tmp = arr[i]; arr[i] = arr[largest]; arr[largest] = tmp;
        i = largest;
    }
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    int* arr = new int[n];
    // LCG pseudo-random fill.
    int v = 42;
    for (int i = 0; i < n; ++i) {
        v = v * 1103515245 + 12345;
        arr[i] = v;
    }

    // Build max-heap — O(N).
    for (int i = n / 2 - 1; i >= 0; --i)
        siftDown(arr, n, i);

    // Extract elements — O(N log N).
    for (int end = n - 1; end > 0; --end) {
        int tmp = arr[0]; arr[0] = arr[end]; arr[end] = tmp;
        siftDown(arr, end, 0);
    }

    sink = arr[0];
    delete[] arr;
    return 0;
}
