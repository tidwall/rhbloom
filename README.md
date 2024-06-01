# Robin hood bloom filter

A [bloom filter](https://en.wikipedia.org/wiki/Bloom_filter) in C that supports dynamic growth using a [robin hood hashmap](https://en.wikipedia.org/wiki/Hash_table#Robin_Hood_hashing).

Basically it starts out as a hashmap and grows until it reaches a capacity that suffices the number of bits to become a full-on bloom filter.

## Example

```c
// Create a filter that has a capacity of 10 million and a false positive
// rate of 0.1%
struct rhbloom rhbloom;
rhbloom_init(&rhbloom, 10000000, 0.001);

// Add the key 12031
rhbloom_add(&rhbloom, 12031);

// Check of the key exists
if (rhbloom_test(&rhbloom, 12031)) {
    // Yes, of course is does.
}

rhbloom_destroy(&rhbloom);
```

## Notes

- The `rhbloom_init()` does not allocate memory.
- The `rhbloom_add()` function may need to allocate memory in order to grow the
data structure. It returns false if the system is out of memory.
- The `rhbloom_add()` function expects that your input key is already hashed.

## Performance

Here we'll benchmark a filter with the capacity of 10,000,000 and a false positive rate of 1%. 

```c
cc -O3 rhbloom.c test.c -lm && ./a.out bench 10000000 0.01
```

```
add          10,000,000 ops in 0.226 secs   22.6 ns/op    44,269,922 op/sec
test (yes)   10,000,000 ops in 0.158 secs   15.8 ns/op    63,427,629 op/sec
test (no)    10,000,000 ops in 0.176 secs   17.6 ns/op    56,667,497 op/sec
Misses 29218 (0.2922% false-positive)
Memory 16.00 MB
```

## License

Source code is available under the MIT License.
