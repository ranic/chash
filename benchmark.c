#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "my_table.h"

#define FNV_SEED 0xcbf29ce484222325
#define FNV_PRIME 0x100000001b3

uint64_t fnv_hash(const char *s, size_t length) {
   // TODO: Loop unroll
   uint64_t hashv = FNV_SEED;
   for (int i = 0; i < length; ++i) {
       hashv ^= (int)s[i];
       hashv *= FNV_PRIME;
   }
   return hashv;
}

void fasthash(const char **wordlist, const int num_words) {
    struct my_table *t = ht_create(10);
    struct my_entry* buffer = (struct my_entry*) calloc(num_words, sizeof(struct my_entry));
    struct my_entry *e = NULL;
    unsigned distinct = 0;

    uint64_t hashv;
    size_t wordlen;

    for (size_t i = 0; i < num_words; ++i) {
        wordlen = strlen(wordlist[i]);
        hashv = fnv_hash(wordlist[i], wordlen);

        if ((e = ht_find(t, hashv, wordlen)) == NULL) {
            e = &buffer[distinct++];
            e->value = 1;
            e->hashv = hashv;
            e->key = wordlist[i];
            e->keylen = wordlen;

            ht_add(t, e);
        } else {
            ++e->value;
        }
    }

    for (size_t i = 0; i < distinct; ++i) {
        e = &buffer[i];
        printf("%s: %zu\n", e->key, e->value);
    }

    ht_destroy(t);
    free(buffer);
}

int main(int argc, char** argv) {
    // Runs and times fasthash by counting uniques in the input file
    int fd;
    // Duplicate the list multiplier times
    int multiplier = 10;
    if (argc <= 1) {
        printf("Must provide input filename as the first argument.\n");
        exit(-1);
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) { exit(1); }
    struct stat stat;
    int rc = fstat(fd, &stat);
    if (rc < 0) { exit(1); }
    const char *words = (const char *)mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    int num_words = 0;
    const char **wordlist = NULL;

    // Read the pointers to the null-delimited words from the file.
    for (int i = 0, j = 0; i < stat.st_size; ++i) {
        if (words[i] == '\0') {
            wordlist = (const char **)realloc(wordlist, ++num_words * sizeof(char *));
            wordlist[num_words - 1] = &words[j];
            j = i + 1;
        }
    }

    wordlist = (const char **)realloc(wordlist, multiplier * num_words * sizeof(char *));
    for (int i = 1; i < multiplier; i++) {
        memcpy(&wordlist[i * num_words], wordlist, num_words * sizeof(char *));
    }
    num_words *= multiplier;

    // Run the solution
    clock_t t1 = clock();
    fasthash(wordlist, num_words);
    printf("------------\n%ld ticks\n", clock() - t1);

    free(wordlist);
    munmap((void *)words, stat.st_size);
    return 0;
}
