// O(log N) — modular exponentiation, amplified
//
// Fast exponentiation is O(log N) multiplications.  We repeat it
// REPS times with fixed REPS so the signal ∝ log(N).
#include <cstdlib>

volatile long long sink;

int main(int argc, char* argv[]) {
    long long n = argc > 1 ? atoll(argv[1]) : 0;
    if (n <= 0) return 0;

    const int REPS = 5000;
    const long long MOD = 1000000007LL;
    long long result = 0;

    for (int r = 0; r < REPS; ++r) {
        long long base = 3, exp = n, val = 1;
        base %= MOD;
        while (exp > 0) {
            if (exp & 1)
                val = val * base % MOD;
            exp >>= 1;
            base = base * base % MOD;
        }
        result += val;
    }
    sink = result;
    return 0;
}
