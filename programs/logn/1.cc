// O(log N) — counting bits / repeated halving
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    // Count how many times we can halve n before it reaches 0.
    // This is exactly floor(log2(n)) + 1 iterations = O(log N).
    int count = 0;
    long long v = n;
    while (v > 0) {
        v >>= 1;
        ++count;
    }
    sink = count;
    return 0;
}
