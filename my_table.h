#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MIN(A, B) ((A) < (B) ? (A) : (B))

/*
 * Simple, high-performance Robin Hood hash table for adding and looking up elements.
 *
 * TODO: Support delete operation and provide a simpler, interface where clients
 * don't manage ht_entry.
 */

struct my_entry {
    const char* key;
    uint64_t hashv;
    size_t value;
    size_t probe_dist;
    size_t keylen;
    // Where this element should have gone
    size_t target_idx;
    // Where the probe of elements that should have hashed to this index
    // actually start.
    size_t target_offset;
};

struct my_table {
    struct my_entry** buckets;
    size_t num_buckets;
    size_t num_buckets_log2;
    size_t size;
    size_t probe_limit;
    size_t size_limit;
};

static struct my_table* ht_create(size_t num_buckets_log2);
static void ht_add(struct my_table *t, struct my_entry *e);
static struct my_entry* ht_find(struct my_table *t, uint64_t hashv, size_t keylen);
void ht_destroy(struct my_table *t);

static inline size_t ht_get_index(struct my_table *t, uint64_t hashv) {
    return hashv & (t->num_buckets - 1);
}

struct my_table* ht_create(size_t num_buckets_log2) {
    struct my_table *t = (struct my_table*) malloc(sizeof(struct my_table));
    if (t == NULL) {
        exit(-1);
    } else if (num_buckets_log2 >= 30) {
        printf("Hash table is way too big!");
    }

    t->num_buckets_log2 = num_buckets_log2;
    t->num_buckets = 1 << t->num_buckets_log2;
    t->buckets = (struct my_entry**) calloc(t->num_buckets, sizeof(struct my_entry*));
    t->size_limit = t->num_buckets * 9 / 10;
    t->size = 0;
    t->probe_limit = 8;

    return t;
}


void ht_grow(struct my_table *t, struct my_entry* e) {
    /** Double the hash table and add entry e for good measure. */
    struct my_entry** old_buckets = t->buckets;
    size_t old_num_buckets = t->num_buckets;

    // Double num_buckets (assume no overflow here)
    ++t->num_buckets_log2;
    t->num_buckets = 1 << t->num_buckets_log2;
    t->buckets = (struct my_entry**) calloc(t->num_buckets, sizeof(struct my_entry*));
    t->size = 0;
    t->size_limit <<= 1;

    // Re-hash all entries into new buckets
    ht_add(t, e);
    for (size_t i = 0; i < old_num_buckets; ++i) {
        e = old_buckets[i];
        if (e) {
            ht_add(t, e);
        }
    }

    free(old_buckets);
}

static void ht_add(struct my_table *t, struct my_entry *e) {
    /* Add entry to the hash table or update an existing entry with the same key with e->value.*/
    size_t mask = t->num_buckets - 1;
    e->target_idx = ht_get_index(t, e->hashv);
    if (t->size >= t->size_limit) {
        return ht_grow(t, e);
    }

    // Start probing at the chain corresponding to this target_idx
    struct my_entry *cur = t->buckets[e->target_idx];
    e->probe_dist = cur ? cur->target_offset : 0;
    size_t idx = (e->target_idx + e->probe_dist) & mask;
    bool replace_target_offset = true;

    for (int i = 0; i < t->probe_limit; ++i) {
        cur = t->buckets[idx];
        replace_target_offset = replace_target_offset && cur->target_idx != e->target_idx;

        if (!cur) {
            // Found a slot, add here
            t->buckets[idx] = e;
            ++t->size;
            if (replace_target_offset) {
                t->buckets[e->target_idx]->target_offset = e->probe_dist;
            }
            return;
        } else {
            // Robin hood case, swap if cur is closer to its start than e
            if (cur->probe_dist < e->probe_dist) {
                // Find cur's target index and increment its offset
                t->buckets[cur->target_idx]->target_offset++;
                // Put e here
                t->buckets[idx] = e;
                if (replace_target_offset) {
                    t->buckets[e->target_idx]->target_offset = e->probe_dist;
                }
                // Pick up cur
                e = cur;
            } else if (cur->hashv == e->hashv && e->keylen == cur->keylen) {
                // Key is already in the hash table, update its value and exit
                cur->value = e->value;
                return;
            }
        }

        ++e->probe_dist;
        idx = (idx + 1) & mask;
    }

    // Double the hash table and try again!
    ht_grow(t, e);
}

static struct my_entry* ht_find(struct my_table *t, uint64_t hashv, size_t keylen) {
    size_t mask = t->num_buckets - 1;
    size_t target_idx = ht_get_index(t, hashv);

    // Start probing at the chain corresponding to this target_idx
    struct my_entry *cur = t->buckets[target_idx];
    if (!cur) return NULL;
    size_t idx = (target_idx + cur->target_offset) & mask;

    // i is the probe length
    for (size_t i = 0; i < t->probe_limit; ++i) {
        cur = t->buckets[idx];

        // Hit the end of the probe
        if (!cur) return NULL;
        // Hit a different chain, stop
        if (target_idx != cur->target_idx) return NULL;

        // Found matching (hash) of element, return entry
        // Assuming here that matching 64-bit hash <==> matching element... #perf
        if (cur->hashv == hashv && keylen == cur->keylen) {
            return cur;
        }

        idx = (idx + 1) & mask;
    }

    return NULL;
}

void ht_destroy(struct my_table *t) {
    printf("Size: %zu\n", t->size);
    for (int i = 0; i < t->num_buckets; ++i) {
        struct my_entry* e = t->buckets[i];
        if (e != NULL) {
            printf("%d: %zu %zu\n", i, e->target_idx, e->target_offset);
        }
    }
    free(t->buckets);
    free(t);
}
