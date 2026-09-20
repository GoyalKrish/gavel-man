#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <N>\n", argv[0]);
        return 1;
    }
    
    // N consideration
    int n = atoi(argv[1]);
    long long sum = 0;
    
    // An O(N) loop
    for (int i = 0; i < n; i++) {
        sum += i;
    }
    
    // An O(N^2) loop to show differences in instructions
    long long pairs = 0;
    for (int i = 0; i < n/10; i++) {
        for (int j = 0; j < n/10; j++) {
            pairs++;
        }
    }
    
    // printf("Sum: %lld", sum);
    printf("Sum: %lld, Pairs: %lld\n", sum, pairs);
    return 0;
}
