// https://github.com/tidwall/rhbloom
//
// Copyright 2024 Joshua J Baker. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.
//
// rhbloom: Robin hood bloom filter

#ifndef RHBLOOM_H
#define RHBLOOM_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

struct rhbloom { _Alignas(16) char _[48]; };

void rhbloom_init(struct rhbloom *rhbloom, size_t n, double p);
void rhbloom_destroy(struct rhbloom *rhbloom);
bool rhbloom_add(struct rhbloom *rhbloom, uint64_t key);
bool rhbloom_test(struct rhbloom *rhbloom, uint64_t key);
size_t rhbloom_memsize(struct rhbloom *rhbloom);
bool rhbloom_upgraded(struct rhbloom *rhbloom);

#endif // RHBLOOM_H
