# Robin hood bloom filter

A [bloom filter](https://en.wikipedia.org/wiki/Bloom_filter) in C that supports dynamic growth using a [robin hood hashmap](https://en.wikipedia.org/wiki/Hash_table#Robin_Hood_hashing).

Basically it starts out as a hashmap and grows until it reaches a capacity that suffices the number of bits to become a full-on bloom filter.

## Example

```c
// Create a filter that has a capacity of 10 million and a false positive
// rate of 0.1%
struct rhbloom *filter = rhbloom_new(10000000, 0.001);

// Add the key 12031
rhbloom_add(filter, 12031);

// Check of the key exists
if (rhbloom_test(filter, 12031)) {
    // Yes, of course is does.
}

rhbloom_free(filter);
```

## API

```c
rhbloom_new(size_t n, double p);              // create a new filter
rhbloom_add(struct rhbloom*, uint64_t key);   // add a key (typically a hash)
rhbloom_test(struct rhbloom*, uint64_t key);  // test if key probably exists
rhbloom_free(struct rhbloom*);                // free the filter
rhbloom_clear(struct rhbloom*);               // clear entries without freeing
```

## Performance

Here we'll benchmark a filter with the capacity of 10,000,000 and a false positive rate of 1%. 

```c
cc -O3 rhbloom.c test.c -lm && ./a.out bench 10000000 0.01
```

```
add          10,000,000 ops in 0.221 secs   22.1 ns/op    45,167,118 op/sec
test (yes)   10,000,000 ops in 0.150 secs   15.0 ns/op    66,673,778 op/sec
test (no)    10,000,000 ops in 0.136 secs   13.6 ns/op    73,576,478 op/sec
Misses 28943 (0.2894% false-positive)
Memory 16.00 MB
```

## License

Source code is available under the MIT License.
