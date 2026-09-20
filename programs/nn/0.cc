// O(N^2) — bubble sort
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    int* arr = new int[n];
    // Reverse-sorted data = worst case for bubble sort.
    for (int i = 0; i < n; ++i) arr[i] = n - i;

    for (int i = 0; i < n - 1; ++i)
        for (int j = 0; j < n - 1 - i; ++j)
            if (arr[j] > arr[j + 1]) {
                int tmp = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = tmp;
            }

    sink = arr[0];
    delete[] arr;
    return 0;
}
