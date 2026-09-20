// O(sqrt N) — trial division, amplified
//
// √4096 = 64 iterations, too small vs overhead.
// Repeat REPS times (fixed constant) to boost signal.
#include <cstdlib>
#include <cmath>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    const int REPS = 2000;
    long long count = 0;
    long long lim = (long long)sqrt((double)n) + 1;

    for (int r = 0; r < REPS; ++r) {
        for (long long i = 2; i <= lim; ++i) {
            if (n % i == 0)
                ++count;
        }
    }
    sink = count;
    return 0;
}
