// O(N!) — generate all permutations of N elements
//
// WARNING: Factorial complexity.  Only feasible for N ≲ 12.
//   ./tester --filter nfact --ns 3,4,5,6,7,8,9,10,11
#include <cstdlib>

volatile long long sink;

static long long count;

static void permute(int* arr, int l, int r) {
    if (l == r) {
        ++count;
        return;
    }
    for (int i = l; i <= r; ++i) {
        // swap arr[l] and arr[i]
        int tmp = arr[l]; arr[l] = arr[i]; arr[i] = tmp;
        permute(arr, l + 1, r);
        // swap back
        tmp = arr[l]; arr[l] = arr[i]; arr[i] = tmp;
    }
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    int* arr = new int[n];
    for (int i = 0; i < n; ++i) arr[i] = i;

    count = 0;
    permute(arr, 0, n - 1);
    sink = count;

    delete[] arr;
    return 0;
}
