// O(sqrt N) — jump search pattern
#include <cstdlib>
#include <cmath>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Simulate a jump search: take sqrt(N) jumps, then do a linear scan
    // of at most sqrt(N) elements.  Total work = O(sqrt N).
    int step = (int)sqrt((double)n);
    if (step < 1) step = 1;

    int sum = 0;
    // Phase 1: sqrt(N) jumps
    int pos = 0;
    while (pos < n) {
        sum += pos;
        pos += step;
    }
    // Phase 2: linear scan within the last block (at most 'step' iterations)
    int start = pos - step;
    if (start < 0) start = 0;
    for (int i = start; i < n && i < start + step; ++i)
        sum += i;

    sink = sum;
    return 0;
}
