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
    size_t count;
    size_t probe_dist;
    size_t keylen;
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
    }

    t->num_buckets_log2 = num_buckets_log2;
    t->num_buckets = 1 << t->num_buckets_log2;
    t->buckets = (struct my_entry**) calloc(t->num_buckets, sizeof(struct my_entry*));
    t->size = 0;
    t->probe_limit = MIN(t->num_buckets, 20);
    t->size_limit = 1 << (num_buckets_log2 - 1);

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
    size_t mask = t->num_buckets - 1;
    size_t idx = ht_get_index(t, e->hashv);
    e->probe_dist = 0;
    if (t->size >= t->size_limit) {
        return ht_grow(t, e);
    }

    while (e->probe_dist < t->probe_limit) {
        struct my_entry* cur = t->buckets[idx];
        if (!cur) {
            // Found a slot, add here
            t->buckets[idx] = e;
            ++t->size;
            return;
        } else {
            // Robin hood case, swap if cur is closer to its start than e
            if (cur->probe_dist < e->probe_dist) {
                // Put e here
                t->buckets[idx] = e;
                // Pick up cur
                e = cur;
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
    size_t idx = ht_get_index(t, hashv);
    struct my_entry* cur;

    // i is the probe length
    for (size_t i = 0; i < t->probe_limit; ++i) {
        cur = t->buckets[idx];

        // Hit the end of the probe
        if (!cur) return NULL;
        // Hit something else with a shorter probe distance, stop
        if (i > cur->probe_dist) return NULL;

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
    free(t);
}
