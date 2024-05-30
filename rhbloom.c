// https://github.com/tidwall/rhbloom
//
// Copyright 2024 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
//
// rhbloom: Robin hood bloom filter

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rhbloom.h"

struct rhbloom0 {
    _Alignas(16)

    // robinhood fields
    size_t count;       // number of keys in hashtable
    size_t nbuckets;    // number of buckets
    uint64_t *buckets;  // hashtable buckets

    // bloom fields
    size_t k;           // number of bits per key
    size_t m;           // number of bits total
    uint8_t *bits;      // bloom bits
};

static_assert(sizeof(struct rhbloom) >= sizeof(struct rhbloom0), "");
static_assert(_Alignof(struct rhbloom) == _Alignof(struct rhbloom0), "");

// dib/key entry as a uint64
#define RHBLOOM_KEY(x) ((uint64_t)(x)<<8>>8)
#define RHBLOOM_DIB(x) (int)((uint64_t)(x)>>56)
#define RHBLOOM_SETKEYDIB(key,dib) (RHBLOOM_KEY((key))|((uint64_t)(dib)<<56))

static void rhbloom_init0(struct rhbloom0 *rhbloom, size_t n, double p) {
    // Calculate the total number of bits needed
    size_t m = n * log(p) / log(1 / pow(2, log(2)));

    // Calculate the bits per key
    size_t k = round(((double)m / (double)(n)) * log(2));

    // Adjust the number of bit to power of two
    size_t m0 = 2;
    while (m0 < m) m0 *= 2;
    size_t k0 = round((double)m / (double)m0 * (double)k);

    rhbloom->k = k0;
    rhbloom->m = m0;
    rhbloom->count = 0;
    rhbloom->nbuckets = 0;
    rhbloom->buckets = 0;
    rhbloom->bits = 0;
}

static void rhbloom_destroy0(struct rhbloom0 *rhbloom) {
    if (rhbloom->bits) {
        free(rhbloom->bits);
    }
    if (rhbloom->buckets) {
        free(rhbloom->buckets);
    }
    rhbloom->count = 0;
    rhbloom->nbuckets = 0;
    rhbloom->buckets = 0;
    rhbloom->bits = 0;
}

static uint64_t rhbloom_mix13(uint64_t key) {
    // https://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html
    // hash u64 using mix13
    key ^= (key >> 30);
    key *= UINT64_C(0xbf58476d1ce4e5b9);
    key ^= (key >> 27);
    key *= UINT64_C(0x94d049bb133111eb);
    key ^= (key >> 31);
    return key;
}

static bool rhbloom_testadd(struct rhbloom0 *rhbloom, uint64_t key, bool add) {
    // We only want the 56-bit key in order to match correcly with the
    // robinhood entries, upon upgrade.
    key = RHBLOOM_KEY(key);

    // Get the bit gap, which is the most signifcant 32-bit value 
    size_t gap = (key >> 24) & (UINT32_MAX-1);

    // Add or check each bit
    for (size_t i = 0; i < rhbloom->k; i++) {
        size_t j = (key+gap*i) % rhbloom->m;
        if (add) {
            rhbloom->bits[j>>3] |= add<<(j&7);
        } else if (!((rhbloom->bits[j>>3]>>(j&7))&1)) {
            return false;
        }
    }
    return true;
}

static bool rhbloom_addkey(struct rhbloom0 *rhbloom, uint64_t key);

static bool rhbloom_grow(struct rhbloom0 *rhbloom) {
    size_t nbuckets_old = rhbloom->nbuckets;
    uint64_t *buckets_old = rhbloom->buckets;
    size_t nbuckets_new = nbuckets_old == 0 ? 16 : nbuckets_old * 2;
    if (nbuckets_new * 8 >= rhbloom->m >> 3) {
        // Upgrade to bloom filter
        rhbloom->bits = calloc(rhbloom->m >> 3, 1);
        if (!rhbloom->bits) {
            return 0;
        }
        rhbloom->count = 0;
        rhbloom->nbuckets = 0;
        rhbloom->buckets = 0;
        for (size_t i = 0; i < nbuckets_old; i++) {
            if (RHBLOOM_DIB(buckets_old[i])) {
                rhbloom_testadd(rhbloom, buckets_old[i], true);
            }
        }
    } else {
        uint64_t *buckets = calloc(nbuckets_new, 8);
        if (!buckets) {
            return 0;
        }
        rhbloom->count = 0;
        rhbloom->nbuckets = nbuckets_new;
        rhbloom->buckets = buckets;
        for (size_t i = 0; i < nbuckets_old; i++) {
            if (RHBLOOM_DIB(buckets_old[i])) {
                rhbloom_addkey(rhbloom, buckets_old[i]);
            }
        }
    }
    if (buckets_old) {
        free(buckets_old);
    }
    return true;
}

static bool rhbloom_addkey(struct rhbloom0 *rhbloom, uint64_t key) {
    key = RHBLOOM_KEY(key);
    int dib = 1;
    size_t i = key & (rhbloom->nbuckets-1);
    while (1) {
        if (RHBLOOM_DIB(rhbloom->buckets[i]) == 0) {
            rhbloom->buckets[i] = RHBLOOM_SETKEYDIB(key, dib);
            rhbloom->count++;
            return true;
        }
        if (RHBLOOM_KEY(rhbloom->buckets[i]) == key) {
            return true;
        }
        if (RHBLOOM_DIB(rhbloom->buckets[i]) < dib) {
            uint64_t tmp = rhbloom->buckets[i];
            rhbloom->buckets[i] = RHBLOOM_SETKEYDIB(key, dib);
            key = RHBLOOM_KEY(tmp);
            dib = RHBLOOM_DIB(tmp);
        }
        dib++;
        i = (i + 1) & (rhbloom->nbuckets-1);
    }
}

static bool rhbloom_add0(struct rhbloom0 *rhbloom, uint64_t key) {
    key = rhbloom_mix13(key);
    while (1) {
        if (rhbloom->bits) {
            rhbloom_testadd(rhbloom, key, true);
            return true;
        }
        if (rhbloom->count == rhbloom->nbuckets >> 1) {
            if (!rhbloom_grow(rhbloom)) {
                return false;
            }
            continue;
        }
        break;
    }
    return rhbloom_addkey(rhbloom, key);
}

static bool rhbloom_test0(struct rhbloom0 *rhbloom, uint64_t key) {
    key = rhbloom_mix13(key);
    if (rhbloom->bits) {
        return rhbloom_testadd(rhbloom, key, false);
    }
    if (!rhbloom->buckets) {
        return false;
    }
    key = RHBLOOM_KEY(key);
    int dib = 1;
    size_t i = key & (rhbloom->nbuckets-1);
    while (1) {
        bool yes = RHBLOOM_KEY(rhbloom->buckets[i]) == key;
        bool no = RHBLOOM_DIB(rhbloom->buckets[i]) < dib;
        if (yes || no) {
            return yes;
        }
        dib++;
        i = (i + 1) & (rhbloom->nbuckets-1);
    }
}

static size_t rhbloom_memsize0(struct rhbloom0 *rhbloom) {
    return rhbloom->bits ? rhbloom->m >> 3 : rhbloom->nbuckets * 8;
}

static bool rhbloom_upgraded0(struct rhbloom0 *rhbloom) {
    return !!rhbloom->bits;
}

//  ** Public API ** //

/// Create a new filter
/// @param n maximum number of keys that can exist in filter to 
/// @return NULL if out of memory
void rhbloom_init(struct rhbloom *rhbloom, size_t n, double p) {
    rhbloom_init0((struct rhbloom0*)rhbloom, n, p);
}

/// Free the filter
void rhbloom_destroy(struct rhbloom *rhbloom) {
    rhbloom_destroy0((struct rhbloom0*)rhbloom);
}

/// Adds a key to the filter.
/// @return true if key was added or false if out of memory 
bool rhbloom_add(struct rhbloom *rhbloom, uint64_t key) {
    return rhbloom_add0((struct rhbloom0*)rhbloom, key);
}

/// Check if key probably exists in filter.
/// @return true if probably exists or false if not exists
bool rhbloom_test(struct rhbloom *rhbloom, uint64_t key) {
    return rhbloom_test0((struct rhbloom0*)rhbloom, key);
}

/// Get the memory size in bytes of this filter.
size_t rhbloom_memsize(struct rhbloom *rhbloom) {
    return rhbloom_memsize0((struct rhbloom0*)rhbloom);
}

/// Returns true if been upgraded to a bloom filter
bool rhbloom_upgraded(struct rhbloom *rhbloom) {
    return rhbloom_upgraded0((struct rhbloom0*)rhbloom);
}