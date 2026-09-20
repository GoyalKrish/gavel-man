// O(log N) — exponentiation by squaring
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    // Compute 3^n mod (10^9 + 7) via fast exponentiation — O(log N) squarings.
    const long long MOD = 1000000007LL;
    long long base = 3, exp = n, result = 1;
    base %= MOD;
    while (exp > 0) {
        if (exp & 1)
            result = result * base % MOD;
        exp >>= 1;
        base = base * base % MOD;
    }
    sink = result;
    return 0;
}
