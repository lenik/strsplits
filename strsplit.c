#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <ctype.h>
#include <getopt.h>
#include "config.h"

#define EXIT_SUCCESS_CODE 0
#define EXIT_INVALID_ARGS 1
#define EXIT_RUNTIME_ERROR 2

#define INITIAL_ITEMS_CAPACITY 64

/* Sort modes */
typedef enum {
    SORT_NONE = 0,
    SORT_IGNORE_CASE,
    SORT_NUMERIC,
    SORT_GENERAL_NUMERIC,
    SORT_HUMAN_NUMERIC,
    SORT_DICTIONARY,
    SORT_VERSION
} SortMode;

/* Configuration structure */
typedef struct {
    char *delim_pattern;      /* Delimiter regex pattern */
    int linenum;              /* Enable line numbering */
    int zero_based;           /* Use 0-based numbering */
    int reset_num;            /* Reset numbering between runs */
    char *format;             /* Output format string */
    int insert_nl_count;      /* Number of blank lines between runs */
    int indent_level;         /* Indentation level (number of -i flags) */
    int reversed;             /* Reverse sort order */
    SortMode sort_mode;       /* Sort mode */
} Config;

/* Item structure for sorting */
typedef struct {
    char *text;
    char *original;           /* Original text (for case-insensitive sort) */
    int index;                /* Original index */
} Item;

/* Global config */
static Config config = {
    .delim_pattern = "\\s+",
    .linenum = 0,
    .zero_based = 0,
    .reset_num = 0,
    .format = NULL,
    .insert_nl_count = 0,
    .indent_level = 0,
    .reversed = 0,
    .sort_mode = SORT_NONE
};

/* Function prototypes */
static void print_usage(FILE *out);
static int parse_args(int argc, char *argv[]);
static Item *split_string(const char *input, regex_t *compiled, int *count);
static void free_items(Item *items, int count);
static int compare_items(const void *a, const void *b);
static void sort_items(Item *items, int count);
static void print_items(Item *items, int count, int start_num, int *current_num);
static int process_run(const char *input, int *current_num, int is_first);
static long human_numeric_value(const char *str);
static int version_compare(const char *a, const char *b);

/* Print usage information */
static void print_usage(FILE *out) {
    fprintf(out, "Usage: strsplit [OPTIONS] ARGS...\n");
    fprintf(out, "\n");
    fprintf(out, "Split input string(s) by delimiter and print each item.\n");
    fprintf(out, "\n");
    fprintf(out, "Options:\n");
    fprintf(out, "  -d, --delim REGEX       Delimiter regex (default: \\\\s+)\n");
    fprintf(out, "  -n, --linenum           Enable line numbering\n");
    fprintf(out, "  -0, --0-based           Use 0-based numbering\n");
    fprintf(out, "  -z, --reset             Reset numbering between runs\n");
    fprintf(out, "  -c, --format FMT        Output format (%%d=index, %%s=item)\n");
    fprintf(out, "  -N, --insert-nl         Insert blank line(s) between runs (repeatable)\n");
    fprintf(out, "  -i, --indented          Indent output by 2 spaces (repeatable)\n");
    fprintf(out, "  -r, --reversed          Reverse sort order\n");
    fprintf(out, "  -f, --ignore-case       Case-insensitive sort\n");
    fprintf(out, "  -s, --numeric-sort      Numeric sort (strtol)\n");
    fprintf(out, "  -g, --general-numeric-sort  General numeric sort (strtod)\n");
    fprintf(out, "  -H, --human-numeric-sort    Human numeric sort (2K, 1M, etc)\n");
    fprintf(out, "  -D, --dictionary-sort   Dictionary sort (ignore non-alnum)\n");
    fprintf(out, "  -V, --version-sort      Version sort (1.9 < 1.10)\n");
    fprintf(out, "      --version           Print version information\n");
    fprintf(out, "      --help              Show this help\n");
}

/* Parse command line arguments */
static int parse_args(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"delim",              required_argument, 0, 'd'},
        {"linenum",            no_argument,       0, 'n'},
        {"0-based",            no_argument,       0, '0'},
        {"reset",              no_argument,       0, 'z'},
        {"format",             required_argument, 0, 'c'},
        {"insert-nl",          no_argument,       0, 'N'},
        {"indented",           no_argument,       0, 'i'},
        {"reversed",           no_argument,       0, 'r'},
        {"ignore-case",        no_argument,       0, 'f'},
        {"numeric-sort",       no_argument,       0, 's'},
        {"general-numeric-sort", no_argument,     0, 'g'},
        {"human-numeric-sort", no_argument,       0, 'H'},
        {"dictionary-sort",    no_argument,       0, 'D'},
        {"version-sort",       no_argument,       0, 'V'},
        {"version",            no_argument,       0, 1000},
        {"help",               no_argument,       0, 1001},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "d:n0zc:NirfsgHDVv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'd':
                config.delim_pattern = optarg;
                break;
            case 'n':
                config.linenum = 1;
                break;
            case '0':
                config.zero_based = 1;
                break;
            case 'z':
                config.reset_num = 1;
                break;
            case 'c':
                config.format = optarg;
                break;
            case 'N':
                config.insert_nl_count++;
                break;
            case 'i':
                config.indent_level++;
                break;
            case 'r':
                config.reversed = 1;
                break;
            case 'f':
                config.sort_mode = SORT_IGNORE_CASE;
                break;
            case 's':
                config.sort_mode = SORT_NUMERIC;
                break;
            case 'g':
                config.sort_mode = SORT_GENERAL_NUMERIC;
                break;
            case 'H':
                config.sort_mode = SORT_HUMAN_NUMERIC;
                break;
            case 'D':
                config.sort_mode = SORT_DICTIONARY;
                break;
            case 'V':
                config.sort_mode = SORT_VERSION;
                break;
            case 1000:
                printf("strsplit %s\n", VERSION);
                printf("author: Lenik (strsplit@bodz.net)\n");
                printf("strsplit is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License.\n");
                exit(EXIT_SUCCESS_CODE);
            case 1001:
                print_usage(stdout);
                exit(EXIT_SUCCESS_CODE);
            default:
                print_usage(stderr);
                return -1;
        }
    }

    /* Set default format if not specified */
    if (!config.format) {
        if (config.linenum) {
            config.format = "%d. %s";
        } else {
            config.format = "%s";
        }
    }

    return optind;
}

/* Extract numeric value for human numeric sort (e.g., "2K" -> 2048) */
static long human_numeric_value(const char *str) {
    char *endptr;
    long val = strtol(str, &endptr, 10);
    
    if (endptr && *endptr) {
        char suffix = toupper((unsigned char)*endptr);
        switch (suffix) {
            case 'K': val *= 1024; break;
            case 'M': val *= 1024 * 1024; break;
            case 'G': val *= 1024 * 1024 * 1024; break;
            case 'T': val *= 1024L * 1024L * 1024L * 1024L; break;
        }
    }
    
    return val;
}

/* Version comparison for version sort */
static int version_compare(const char *a, const char *b) {
    while (*a || *b) {
        /* Skip non-digits */
        while (*a && !isdigit((unsigned char)*a)) a++;
        while (*b && !isdigit((unsigned char)*b)) b++;
        
        /* Extract numbers */
        char *end_a, *end_b;
        long num_a = (*a) ? strtol(a, &end_a, 10) : 0;
        long num_b = (*b) ? strtol(b, &end_b, 10) : 0;
        
        if (num_a != num_b) {
            return (num_a > num_b) ? 1 : -1;
        }
        
        a = end_a;
        b = end_b;
    }
    
    return 0;
}

/* Comparison function for qsort */
static int compare_items(const void *a, const void *b) {
    const Item *ia = (const Item *)a;
    const Item *ib = (const Item *)b;
    
    int result = 0;
    
    switch (config.sort_mode) {
        case SORT_IGNORE_CASE:
            result = strcasecmp(ia->text, ib->text);
            break;
        case SORT_NUMERIC: {
            char *end_a, *end_b;
            long val_a = strtol(ia->text, &end_a, 10);
            long val_b = strtol(ib->text, &end_b, 10);
            if (!*end_a && !*end_b) {
                /* Both are purely numeric, compare values */
                result = (val_a > val_b) - (val_a < val_b);
            } else {
                /* Fall back to string comparison */
                result = strcmp(ia->text, ib->text);
            }
            break;
        }
        case SORT_GENERAL_NUMERIC: {
            char *end_a, *end_b;
            double val_a = strtod(ia->text, &end_a);
            double val_b = strtod(ib->text, &end_b);
            result = (val_a > val_b) - (val_a < val_b);
            break;
        }
        case SORT_HUMAN_NUMERIC: {
            long val_a = human_numeric_value(ia->text);
            long val_b = human_numeric_value(ib->text);
            result = (val_a > val_b) - (val_a < val_b);
            break;
        }
        case SORT_DICTIONARY: {
            /* Compare ignoring non-alphanumeric characters */
            const char *pa = ia->text;
            const char *pb = ib->text;
            while (*pa || *pb) {
                while (*pa && !isalnum((unsigned char)*pa)) pa++;
                while (*pb && !isalnum((unsigned char)*pb)) pb++;
                if (toupper((unsigned char)*pa) != toupper((unsigned char)*pb)) {
                    result = toupper((unsigned char)*pa) - toupper((unsigned char)*pb);
                    break;
                }
                if (*pa) pa++;
                if (*pb) pb++;
            }
            break;
        }
        case SORT_VERSION:
            result = version_compare(ia->text, ib->text);
            break;
        case SORT_NONE:
        default:
            /* Preserve original order */
            result = ia->index - ib->index;
            break;
    }
    
    if (config.reversed) {
        result = -result;
    }
    
    return result;
}

/* Sort items array */
static void sort_items(Item *items, int count) {
    if (count <= 1) return;
    
    if (config.sort_mode == SORT_NONE && !config.reversed) {
        /* No sorting needed, preserve order */
        return;
    }
    
    qsort(items, count, sizeof(Item), compare_items);
}

/* Split input string by delimiter regex */
static Item *split_string(const char *input, regex_t *compiled, int *count) {
    *count = 0;
    
    if (!input || !*input) {
        return NULL;
    }
    
    Item *items = NULL;
    int capacity = INITIAL_ITEMS_CAPACITY;
    int idx = 0;
    
    items = malloc(capacity * sizeof(Item));
    if (!items) {
        return NULL;
    }
    
    const char *end = input;
    regmatch_t pmatch[1];
    
    while (*end) {
        int ret = regexec(compiled, end, 1, pmatch, 0);
        
        if (ret == REG_NOMATCH) {
            /* No more matches, take rest of string */
            size_t len = strlen(end);
            if (len > 0 || idx == 0) {
                /* Grow array if needed */
                if (idx >= capacity) {
                    capacity *= 2;
                    Item *new_items = realloc(items, capacity * sizeof(Item));
                    if (!new_items) {
                        free_items(items, idx);
                        return NULL;
                    }
                    items = new_items;
                }
                
                items[idx].text = strndup(end, len);
                items[idx].original = items[idx].text;
                items[idx].index = idx;
                idx++;
            }
            break;
        } else if (ret == 0) {
            /* Match found */
            size_t match_start = (size_t)(end - input + pmatch[0].rm_so);
            size_t match_len = (size_t)(pmatch[0].rm_eo - pmatch[0].rm_so);
            
            /* Extract item before delimiter */
            size_t item_len = match_start - (size_t)(end - input);
            
            /* Grow array if needed */
            if (idx >= capacity) {
                capacity *= 2;
                Item *new_items = realloc(items, capacity * sizeof(Item));
                if (!new_items) {
                    free_items(items, idx);
                    return NULL;
                }
                items = new_items;
            }
            
            items[idx].text = strndup(end, item_len);
            items[idx].original = items[idx].text;
            items[idx].index = idx;
            idx++;
            
            /* Move past delimiter */
            end = input + match_start + match_len;
            
            /* Handle empty item at end (delimiter at end of string) */
            if (*end == '\0' && match_len > 0) {
                /* Trailing delimiter produces empty item */
                if (idx >= capacity) {
                    capacity *= 2;
                    Item *new_items = realloc(items, capacity * sizeof(Item));
                    if (!new_items) {
                        free_items(items, idx);
                        return NULL;
                    }
                    items = new_items;
                }
                items[idx].text = strdup("");
                items[idx].original = items[idx].text;
                items[idx].index = idx;
                idx++;
            }
        } else {
            /* Regex error */
            free_items(items, idx);
            return NULL;
        }
    }
    
    *count = idx;
    return items;
}

/* Free items array */
static void free_items(Item *items, int count) {
    if (!items) return;
    
    for (int i = 0; i < count; i++) {
        free(items[i].text);
    }
    free(items);
}

/* Print items with formatting */
static void print_items(Item *items, int count, int start_num, int *current_num) {
    if (!items || count <= 0) return;
    
    for (int i = 0; i < count; i++) {
        /* Print indentation */
        for (int j = 0; j < config.indent_level; j++) {
            printf("  ");
        }
        
        /* Print formatted output */
        int num = config.zero_based ? (start_num + i) : (start_num + i + 1);
        
        if (config.linenum) {
            /* Replace %d and %s in format string */
            const char *fmt = config.format;
            while (*fmt) {
                if (*fmt == '%' && *(fmt + 1) == 'd') {
                    printf("%d", num);
                    fmt += 2;
                } else if (*fmt == '%' && *(fmt + 1) == 's') {
                    printf("%s", items[i].text);
                    fmt += 2;
                } else {
                    putchar(*fmt);
                    fmt++;
                }
            }
        } else {
            /* No line number, just use format with %s */
            const char *fmt = config.format;
            while (*fmt) {
                if (*fmt == '%' && *(fmt + 1) == 's') {
                    printf("%s", items[i].text);
                    fmt += 2;
                } else if (*fmt == '%') {
                    /* Pass through to printf for other format specifiers */
                    putchar(*fmt);
                    fmt++;
                    if (*fmt) {
                        putchar(*fmt);
                        fmt++;
                    }
                } else {
                    putchar(*fmt);
                    fmt++;
                }
            }
        }
        
        putchar('\n');
    }
    
    if (current_num) {
        *current_num += count;
    }
}

/* Process one run (one input string) */
static int process_run(const char *input, int *current_num, int is_first) {
    if (!input || !*input) {
        return 0;
    }
    
    /* Compile delimiter regex */
    regex_t compiled;
    int ret = regcomp(&compiled, config.delim_pattern, REG_EXTENDED);
    if (ret != 0) {
        fprintf(stderr, "Error: Invalid regex pattern\n");
        return EXIT_RUNTIME_ERROR;
    }
    
    /* Split input */
    int count = 0;
    Item *items = split_string(input, &compiled, &count);
    
    regfree(&compiled);
    
    if (!items || count == 0) {
        return 0;
    }
    
    /* Sort items */
    sort_items(items, count);
    
    /* Print separator before this run (if not first) */
    if (!is_first && config.insert_nl_count > 0) {
        for (int i = 0; i < config.insert_nl_count; i++) {
            putchar('\n');
        }
    }
    
    /* Determine starting number for this run */
    int start_num = 0;
    if (config.reset_num) {
        start_num = 0;
    } else {
        start_num = *current_num;
    }
    
    /* Print items */
    print_items(items, count, start_num, current_num);
    
    /* Free items */
    free_items(items, count);
    
    return 0;
}

int main(int argc, char *argv[]) {
    /* Parse args - returns negative on error, otherwise returns optind */
    int arg_start = parse_args(argc, argv);
    
    if (arg_start < 0) {
        return EXIT_INVALID_ARGS;
    }
    
    int current_num = 0;
    int is_first = 1;
    int exit_code = EXIT_SUCCESS_CODE;
    
    if (argc > arg_start) {
        /* Process command line arguments */
        for (int i = arg_start; i < argc; i++) {
            int ret = process_run(argv[i], &current_num, is_first);
            if (ret != 0) {
                exit_code = ret;
                break;
            }
            is_first = 0;
        }
    } else {
        /* Read from stdin */
        char *line = NULL;
        size_t len = 0;
        ssize_t read;
        
        while ((read = getline(&line, &len, stdin)) != -1) {
            /* Remove trailing newline */
            if (read > 0 && line[read - 1] == '\n') {
                line[read - 1] = '\0';
            }
            
            int ret = process_run(line, &current_num, is_first);
            if (ret != 0) {
                exit_code = ret;
                break;
            }
            is_first = 0;
        }
        
        free(line);
    }
    
    return exit_code;
}
