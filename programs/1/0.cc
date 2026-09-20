// O(1) — constant-time: fixed arithmetic regardless of N
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    (void)n;

    // A fixed amount of work, independent of N.
    int x = 42;
    for (int i = 0; i < 100; ++i)
        x = x * 31 + 17;
    sink = x;
    return 0;
}
