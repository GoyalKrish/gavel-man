// O(N log N) — merge sort
#include <cstdlib>

volatile int sink;

static void merge(int* arr, int* tmp, int lo, int mid, int hi) {
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi)
        tmp[k++] = (arr[i] <= arr[j]) ? arr[i++] : arr[j++];
    while (i < mid) tmp[k++] = arr[i++];
    while (j < hi)  tmp[k++] = arr[j++];
    for (int x = lo; x < hi; ++x) arr[x] = tmp[x];
}

static void mergeSort(int* arr, int* tmp, int lo, int hi) {
    if (hi - lo <= 1) return;
    int mid = lo + (hi - lo) / 2;
    mergeSort(arr, tmp, lo, mid);
    mergeSort(arr, tmp, mid, hi);
    merge(arr, tmp, lo, mid, hi);
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    int* arr = new int[n];
    int* tmp = new int[n];
    // Fill with reverse-sorted data to guarantee worst-case merges.
    for (int i = 0; i < n; ++i) arr[i] = n - i;

    mergeSort(arr, tmp, 0, n);

    sink = arr[0];
    delete[] arr;
    delete[] tmp;
    return 0;
}
