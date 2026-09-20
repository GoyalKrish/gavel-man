// O(sqrt N) — explicit sqrt(N) iterations, amplified
//
// Count from 1 to floor(sqrt(N)), doing fixed work per step.
// Repeat REPS times (fixed) to amplify the signal.
#include <cstdlib>
#include <cmath>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    const int REPS = 2000;
    long long lim = (long long)sqrt((double)n);
    if (lim < 1) lim = 1;

    long long total = 0;
    for (int r = 0; r < REPS; ++r) {
        for (long long i = 1; i <= lim; ++i) {
            total += i * i + 3;
        }
    }
    sink = total;
    return 0;
}
