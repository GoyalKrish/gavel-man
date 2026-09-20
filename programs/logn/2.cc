// O(log N) — binary GCD (Stein's algorithm), amplified
//
// Stein's algorithm computes gcd in O(log N) steps.
// Repeated REPS times to amplify the signal.
#include <cstdlib>

volatile long long sink;

static long long binaryGcd(long long a, long long b) {
    if (a == 0) return b;
    if (b == 0) return a;
    int shift = __builtin_ctzll(a | b);
    a >>= __builtin_ctzll(a);
    do {
        b >>= __builtin_ctzll(b);
        if (a > b) { long long t = a; a = b; b = t; }
        b -= a;
    } while (b != 0);
    return a << shift;
}

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    const int REPS = 5000;
    long long total = 0;

    for (int r = 0; r < REPS; ++r) {
        // Compute gcd(n, n-1) — these are always coprime so the
        // algorithm runs the full O(log N) depth.
        total += binaryGcd(n, n > 1 ? n - 1 : 1);
    }
    sink = total;
    return 0;
}
