// O(N^3) — Floyd-Warshall all-pairs shortest paths
//
// Run with smaller N values to avoid timeout: --ns 32,64,128,256,512
#include <cstdlib>

volatile int sink;

int main(int argc, char* argv[]) {
    int n = argc > 1 ? atoi(argv[1]) : 0;
    if (n <= 0) return 0;

    // Build a pseudo-random adjacency matrix, then run Floyd-Warshall.
    int* dist = new int[n * n];
    int v = 5;
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
            if (i == j) { dist[i * n + j] = 0; continue; }
            v = v * 1103515245 + 12345;
            dist[i * n + j] = (v & 0x7FFF) % 1000 + 1;
        }

    // Floyd-Warshall — O(N^3).
    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j) {
                int through = dist[i * n + k] + dist[k * n + j];
                if (through < dist[i * n + j])
                    dist[i * n + j] = through;
            }

    sink = dist[0 * n + (n > 1 ? 1 : 0)];
    delete[] dist;
    return 0;
}
