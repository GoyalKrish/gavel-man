// O(sqrt N) — trial division to check primality
#include <cstdlib>
#include <cmath>

volatile int sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    // Check divisibility of n by every integer up to sqrt(n).
    // This is O(sqrt N) iterations.
    int count = 0;
    long long lim = (long long)sqrt((double)n) + 1;
    for (long long i = 2; i <= lim; ++i) {
        if (n % i == 0)
            ++count;
    }
    sink = count;
    return 0;
}
