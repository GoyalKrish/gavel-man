// O(sqrt N) — sum of divisors via sqrt decomposition, amplified
#include <cstdlib>
#include <cmath>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    const int REPS = 2000;
    long long total = 0;
    long long lim = (long long)sqrt((double)n) + 1;

    for (int r = 0; r < REPS; ++r) {
        long long sum = 0;
        for (long long i = 1; i <= lim; ++i) {
            if (n % i == 0) {
                sum += i;
                if (i != n / i)
                    sum += n / i;
            }
        }
        total += sum;
    }
    sink = total;
    return 0;
}
