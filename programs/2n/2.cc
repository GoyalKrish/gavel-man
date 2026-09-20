// O(2^N) — Tower of Hanoi (2^N - 1 moves)
//
// WARNING: Exponential complexity.  Only feasible for N ≲ 28.
//   ./tester --filter 2n --ns 4,8,12,16,20,24
#include <cstdlib>

volatile long long sink;

static long long hanoi(int n, int from, int to, int aux) {
    if (n <= 0) return 0;
    long long moves = 0;
    moves += hanoi(n - 1, from, aux, to);
    ++moves;   // move disk n
    moves += hanoi(n - 1, aux, to, from);
    return moves;
}

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    sink = hanoi(n, 0, 2, 1);
    return 0;
}
