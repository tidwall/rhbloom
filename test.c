// # Run tests
// $ cc *.c && ./a.out
// 
// # run benchmarks
// $ cc *.c && ./a.out bench

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "rhbloom.h"

unsigned int murmurhash2(const void * key, int len, const unsigned int seed) {
    const unsigned int m = 0x5bd1e995;
    const int r = 24;
    unsigned int h = seed ^ len;
    const unsigned char * data = (const unsigned char *)key;
    while(len >= 4) {
        unsigned int k = *(unsigned int *)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        len -= 4;
    }   
    switch(len) {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= m;
    };  
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

uint64_t hash(int x) {
    return murmurhash2(&x, sizeof(int), 0);
}

double now(void) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec*1e9 + now.tv_nsec) / 1e9;
}

char *commaize(unsigned int n) {
    char s1[64];
    char *s2 = malloc(64);
    assert(s2);
    memset(s2, 0, sizeof(64));
    snprintf(s1, sizeof(s1), "%d", n);
    int i = strlen(s1)-1; 
    int j = 0;
    while (i >= 0) {
        if (j%3 == 0 && j != 0) {
            memmove(s2+1, s2, strlen(s2)+1);
            s2[0] = ',';
    }
        memmove(s2+1, s2, strlen(s2)+1);
        s2[0] = s1[i];
        i--;
        j++;
    }
    return s2;
}

#define bench_print(n, start, end) { \
    double elapsed = end - start; \
    double nsop = elapsed/(double)(n)*1e9; \
    char *pops = commaize((n)); \
    char *psec = commaize((double)(n)/elapsed); \
    printf("%s ops in %.3f secs %6.1f ns/op %13s op/sec\n", \
        pops, elapsed, nsop, psec); \
}

void test_step(struct rhbloom *rhbloom, int n, double p) {
    int nn = n+1;
    for (int i = 0; i < nn; i++) {
        if (!rhbloom_upgraded(rhbloom)) {
            assert(!rhbloom_test(rhbloom, hash(i)));
        }
        rhbloom_add(rhbloom, hash(i));
        if (!rhbloom_upgraded(rhbloom)) {
            assert(rhbloom_test(rhbloom, hash(i)));
        }
    }
    assert(rhbloom_upgraded(rhbloom));
    int hits = 0;
    for (int i = 0; i < nn; i++) {
        if (rhbloom_test(rhbloom, hash(i))) {
            hits++;
        }
    }
    assert(hits == nn);
    hits = 0;

    for (int i = nn; i < nn*2; i++) {
        if (rhbloom_test(rhbloom, hash(i))) {
            hits++;
        }
    }
    if ((double)hits/(double)n - p > 0.1 && n > 0) {
        printf("n=%d p=%f hits=%d \t(%f)", 
            n, p, hits, (double)hits/(double)n);
        printf(" (%f)", (double)hits/(double)n - p);
        printf("\n");
        assert(!"bad probability");
    }
}

void test(void) {
    for (int n = 0; n < 100000; n += 1000) {
        for (double p = 0.01; p < 0.70; p += 0.05) {
            struct rhbloom *rhbloom = rhbloom_new(n, p);
            assert(rhbloom);
            test_step(rhbloom, n, p);
            // test after clear
            rhbloom_clear(rhbloom); 
            test_step(rhbloom, n, p);
            rhbloom_free(rhbloom);
        }
    }
    printf("PASSED\n");
}

void bench(int argc, char *argv[]) {

    int N = 1000000;
    double P = 0.01;


    if (argc > 2) {
        N = atoi(argv[2]);
    }
    if (argc > 3) {
        P = atof(argv[3]);
    }

    uint64_t *hashes = malloc(N*8*2);
    assert(hashes);
    for (int i = 0; i < N*2; i++) {
        hashes[i] = hash(i);
    }

    struct rhbloom *rhbloom = rhbloom_new(N, P);
    assert(rhbloom);
// exit(1);
    double start;

    size_t misses = 0;
    for (int j = 0; j < 2; j++) {
        if (j > 0) {
            printf("-- clear --\n");
            rhbloom_clear(rhbloom);
        }
        printf("add          ");
        start = now();
        for (int i = 0; i < N; i++) {
            // printf("insert %d (%llu)\n", i, hash(i));
            rhbloom_add(rhbloom, hashes[i]);
        }
        bench_print(N, start, now());

        // rhbloom_print(rhbloom);

        printf("test (yes)   ");
        start = now();
        for (int i = 0; i < N; i++) {
            // printf("contains %d (%llu)\n", i, hash(i));
            assert(rhbloom_test(rhbloom, hashes[i]));
        }
        bench_print(N, start, now());


        printf("test (no)    ");
        misses = 0;
        start = now();
        for (int i = N; i < N*2; i++) {
            misses += rhbloom_test(rhbloom, hashes[i]);
        }
        bench_print(N, start, now());

    }


    printf("Misses %zu (%0.4f%% false-positive)\n", misses, misses / (double)N * 100);
    printf("Memory %.2f MB\n", rhbloom_memsize(rhbloom)/1024.0/1024.0);

    rhbloom_free(rhbloom);
    free(hashes);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "bench") == 0) {
        bench(argc, argv);
    } else {
        test();
    }
    return 0;
}