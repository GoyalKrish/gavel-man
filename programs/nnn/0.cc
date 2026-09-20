// O(N^3) — naive matrix multiplication
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Allocate three n×n matrices (flat arrays).
    int* A = new int[n * n];
    int* B = new int[n * n];
    int* C = new int[n * n];

    // Initialise A and B with simple values.
    for (int i = 0; i < n * n; ++i) {
        A[i] = i % 17 + 1;
        B[i] = (i * 3) % 13 + 1;
        C[i] = 0;
    }

    // C = A * B  —  O(N^3) multiplications and additions.
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            long long s = 0;
            for (int k = 0; k < n; ++k)
                s += (long long)A[i * n + k] * B[k * n + j];
            C[i * n + j] = (int)s;
        }

    sink = C[0];
    delete[] A;
    delete[] B;
    delete[] C;
    return 0;
}
