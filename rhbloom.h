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

struct rhbloom;

struct rhbloom *rhbloom_new(size_t n, double p);
struct rhbloom *rhbloom_new_with_allocator(size_t n, double p, void*(*malloc)(size_t), void(*free)(void*));
void rhbloom_free(struct rhbloom *rhbloom);
void rhbloom_clear(struct rhbloom *rhbloom);
bool rhbloom_add(struct rhbloom *rhbloom, uint64_t key);
bool rhbloom_test(struct rhbloom *rhbloom, uint64_t key);
size_t rhbloom_memsize(struct rhbloom *rhbloom);
bool rhbloom_upgraded(struct rhbloom *rhbloom);

#endif // RHBLOOM_H
