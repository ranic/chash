import argparse
import random
import string
import sys

NULL_BYTE = '\0'

def gen_random_string(max_length):
    l = random.randint(1, max_length)
    return ''.join(random.choice(string.ascii_letters) for _ in xrange(l))


def generate(N, C, L, newline):
    """Generate N words with cardinality C and max length L"""
    choices = set()
    while len(choices) < C:
        choices.add(gen_random_string(L))

    choices = list(choices)
    sep = '\n' if newline else NULL_BYTE
    for i in xrange(N):
        sys.stdout.write('%s%s' % (choices[i % C], sep))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Write a list of null-byte delimited words to stdout')
    parser.add_argument('num_words', type=int, help='Number of words to generate.')
    parser.add_argument('--cardinality', type=int, help='Number of distinct words to generate.')
    parser.add_argument('--max-word-size', type=int, default=32, help='Biggest word to generate.')
    parser.add_argument('--seed', type=int, default=random.randint(0,10000), help='Integer seed to get deterministic results.')
    parser.add_argument('--newline', action='store_true', help='Delimit strings with newlines')

    args = parser.parse_args()

    # Default to all distinct words
    if not args.cardinality or args.cardinality > args.num_words:
        args.cardinality = num_words

    print>>sys.stderr, 'Generating random words with seed: %d' % args.seed
    random.seed(args.seed)

    generate(args.num_words, args.cardinality, args.max_word_size, args.newline)
