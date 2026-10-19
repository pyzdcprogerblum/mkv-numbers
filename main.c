#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { BASE = 10 };

typedef struct {
    int digits;
    int sum;
    bool count_only;
    bool help;
} Options;

/* Accept only nonempty ASCII decimal strings; reject overflow and range errors. */
static bool parse_integer(const char *text, int minimum, int maximum, int *value)
{
    if (*text == '\0') {
        return false;
    }
    for (const char *p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return false;
        }
    }
    errno = 0;
    char *end = NULL;
    long parsed = strtol(text, &end, 10);
    if (errno == ERANGE || *end != '\0' || parsed < minimum || parsed > maximum) {
        return false;
    }
    *value = (int)parsed;
    return true;
}

/* Validate every argument before enumeration or help output begins. */
static bool parse_options(int argc, char **argv, Options *options)
{
    bool seen_digits = false;
    bool seen_sum = false;
    *options = (Options){4, 10, false, false};

    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0) {
            if (argc != 2) {
                fputs("error: --help must be used alone\n", stderr);
                return false;
            }
            options->help = true;
        } else if (strcmp(arg, "--count-only") == 0) {
            if (options->count_only) {
                fputs("error: duplicate --count-only\n", stderr);
                return false;
            }
            options->count_only = true;
        } else if (strcmp(arg, "--digits") == 0 || strcmp(arg, "--sum") == 0) {
            bool is_digits = strcmp(arg, "--digits") == 0;
            bool *seen = is_digits ? &seen_digits : &seen_sum;
            if (*seen) {
                fprintf(stderr, "error: duplicate %s\n", arg);
                return false;
            }
            *seen = true;
            if (i + 1 == argc) {
                fprintf(stderr, "error: %s requires a value\n", arg);
                return false;
            }
            int *value = is_digits ? &options->digits : &options->sum;
            int minimum = is_digits ? 2 : 9;
            if (!parse_integer(argv[++i], minimum, 10, value)) {
                fprintf(stderr, "error: %s requires decimal digits in range %s\n",
                        arg, is_digits ? "2..10" : "9..10");
                return false;
            }
        } else {
            fprintf(stderr, "error: unknown option or positional argument: %s\n", arg);
            return false;
        }
    }
    return true;
}

/* Replace a distinct-element permutation by its immediate lexicographic successor.
 * Return false without modifying it when its order is entirely decreasing. */
static bool next_permutation(int permutation[BASE])
{
    int pivot = BASE - 2;
    while (pivot >= 0 && permutation[pivot] > permutation[pivot + 1]) {
        --pivot;
    }
    if (pivot < 0) {
        return false;
    }
    int successor = BASE - 1;
    while (permutation[successor] < permutation[pivot]) {
        --successor;
    }
    int temporary = permutation[pivot];
    permutation[pivot] = permutation[successor];
    permutation[successor] = temporary;
    for (int left = pivot + 1, right = BASE - 1; left < right; ++left, --right) {
        temporary = permutation[left];
        permutation[left] = permutation[right];
        permutation[right] = temporary;
    }
    return true;
}

/* Precondition: 2 <= length <= 10, distinct digits in 0..9.
 * Lecture's (+, L, -): append missing digits in decreasing order,
 * take next full permutation, retain its first length digits.
 * On false, arrangement remains unchanged. */
static bool next_arrangement(int arrangement[BASE], int length)
{
    bool used[BASE] = {false};
    int permutation[BASE];
    for (int i = 0; i < length; ++i) {
        permutation[i] = arrangement[i];
        used[arrangement[i]] = true;
    }
    int position = length;
    for (int digit = BASE - 1; digit >= 0; --digit) {
        if (!used[digit]) {
            permutation[position++] = digit;
        }
    }
    if (!next_permutation(permutation)) {
        return false;
    }
    for (int i = 0; i < length; ++i) {
        arrangement[i] = permutation[i];
    }
    return true;
}

/* Test existence, not number, adjacency, or positions of complementary pairs. */
static bool has_complementary_pair(const int digits[BASE], int length, int sum)
{
    for (int i = 0; i < length; ++i) {
        for (int j = i + 1; j < length; ++j) {
            if (digits[i] + digits[j] == sum) {
                return true;
            }
        }
    }
    return false;
}

/* Emit digits directly, including ten-digit values beyond signed 32-bit range. */
static bool write_number(const int digits[BASE], int length)
{
    char line[BASE + 1];
    for (int i = 0; i < length; ++i) {
        line[i] = (char)('0' + digits[i]);
    }
    line[length] = '\n';
    size_t size = (size_t)length + 1;
    return fwrite(line, 1, size, stdout) == size;
}

static bool write_help(void)
{
    return fputs(
        "Enumerate decimal natural numbers with distinct digits and at least\n"
        "one pair of positions i < j whose digits sum to S. Output is ascending.\n"
        "Usage: mkv_numbers [--digits M] [--sum S] [--count-only]\n"
        "       mkv_numbers --help\n"
        "  --digits M     Number of decimal digits: 2..10 (default: 4).\n"
        "  --sum S        Complementary digit sum: 9 or 10 (default: 10).\n"
        "  --count-only   Enumerate all candidates; print only result count.\n"
        "  --help         Show this help; must be used alone.\n"
        "Values: nonempty ASCII decimal digit strings, separate arguments.\n"
        "Leading zeroes in parameter values are accepted. No repeated options.\n"
        "Output: one number per line, or one count followed by a newline.\n"
        "Examples:\n"
        "  ./mkv_numbers --digits 2 --sum 10\n"
        "  ./mkv_numbers --digits 4 --sum 10 --count-only\n"
        "  ./mkv_numbers --digits 10 --sum 9 > numbers.txt\n", stdout) >= 0;
}

int main(int argc, char **argv)
{
    Options options;
    if (!parse_options(argc, argv, &options)) {
        return 2;
    }
    bool output_ok = true;
    if (options.help) {
        output_ok = write_help();
    } else {
        int digits[BASE] = {0};
        for (int i = 0; i < options.digits; ++i) {
            digits[i] = i;
        }
        uint64_t count = 0;
        do {
            if (digits[0] != 0 &&
                has_complementary_pair(digits, options.digits, options.sum)) {
                ++count;
                if (!options.count_only && !write_number(digits, options.digits)) {
                    output_ok = false;
                    break;
                }
            }
        } while (next_arrangement(digits, options.digits));
        if (options.count_only) {
            output_ok = printf("%" PRIu64 "\n", count) >= 0;
        }
    }
    /* Buffered writes can fail only when the final buffer is flushed. */
    if (fflush(stdout) == EOF) {
        output_ok = false;
    }
    if (!output_ok) {
        fputs("error: cannot write to stdout\n", stderr);
        return 1;
    }
    return 0;
}
