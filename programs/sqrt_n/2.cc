// O(sqrt N) — sum of divisors using the sqrt-factorisation trick
#include <cstdlib>
#include <cmath>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    // Sum all divisors of n.  We iterate i from 1 to sqrt(n) and check
    // n % i == 0, adding both i and n/i.  O(sqrt N) work.
    long long sum = 0;
    long long lim = (long long)sqrt((double)n) + 1;
    for (long long i = 1; i <= lim; ++i) {
        if (n % i == 0) {
            sum += i;
            if (i != n / i)
                sum += n / i;
        }
    }
    sink = sum;
    return 0;
}
