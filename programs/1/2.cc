// O(1) — constant-time: array fill of fixed size
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    (void)n;

    // Fixed-size array work — does not depend on N.
    int arr[50];
    for (int i = 0; i < 50; ++i)
        arr[i] = i * i + 3;
    int sum = 0;
    for (int i = 0; i < 50; ++i)
        sum += arr[i];
    sink = sum;
    return 0;
}
