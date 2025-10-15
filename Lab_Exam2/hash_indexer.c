/*
 * hash_indexer.c
 *
 * Indexes words in a text file using a hash table with Robin Hood linear probing.
 *
 * Usage:
 *   ./hash_indexer <input_file> <index_output_file> <table_size>
 *
 * Example compile and run:
 *   gcc -O2 -o hash_indexer hash_indexer.c
 *   ./hash_indexer input.txt index_output.txt 200003
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define LOAD_FACTOR_THRESHOLD 0.9
#define INITIAL_LINES_CAPACITY 4


// Entry struct for hash table
typedef struct {
    char *word;            // dynamically allocated string (NULL if empty)
    int *lines;            // dynamic array of stored line numbers
    int lines_count;
    int lines_capacity;
    int probe_distance;    // number of steps from initial hashed index
    int occupied;          // 0 or 1
} Entry;


// Simple DJB2 hash function
unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}


// Append line number to entry's lines array (no duplicates)
int append_line_to_entry(Entry *e, int line) {
    if (e->lines_count > 0 && e->lines[e->lines_count - 1] == line) {
        return 0; // already present for this line
    }
    if (e->lines_count == e->lines_capacity) {
        int new_cap = e->lines_capacity ? e->lines_capacity * 2 : INITIAL_LINES_CAPACITY;
        int *new_lines = realloc(e->lines, new_cap * sizeof(int));
        if (!new_lines) return -1;
        e->lines = new_lines;
        e->lines_capacity = new_cap;
    }
    e->lines[e->lines_count++] = line;
    return 1;
}


// Insert or update word in hash table with Robin Hood linear probing
int insert_word(Entry *table, int table_size, const char *word, int line, int *occupied_count) {
    unsigned long h = hash_djb2(word);
    int ideal = h % table_size;
    int idx = ideal;
    int probe_dist = 0;
    // Prepare new entry
    Entry incoming = {NULL, NULL, 0, 0, probe_dist, 1};
    incoming.word = strdup(word);
    if (!incoming.word) return -1;
    if (append_line_to_entry(&incoming, line) < 0) {
        free(incoming.word);
        return -1;
    }
    while (1) {
        Entry *slot = &table[idx];
        if (!slot->occupied) {
            // Place incoming here
            *slot = incoming;
            (*occupied_count)++;
            return 1;
        } else if (strcmp(slot->word, word) == 0) {
            // Word exists, append line if needed
            int res = append_line_to_entry(slot, line);
            free(incoming.word);
            free(incoming.lines);
            return res;
        } else {
            // Robin Hood: swap if incoming's probe_dist > slot's
            int slot_ideal = hash_djb2(slot->word) % table_size;
            int slot_probe = (idx - slot_ideal + table_size) % table_size;
            if (slot_probe < probe_dist) {
                // Swap slot <-> incoming
                Entry tmp = *slot;
                *slot = incoming;
                incoming = tmp;
                probe_dist = slot_probe;
            }
        }
        idx = (idx + 1) % table_size;
        probe_dist++;
        if (probe_dist >= table_size) {
            // Table full
            free(incoming.word);
            free(incoming.lines);
            return -2;
        }
    }
}


// Free all memory in table
void free_table(Entry *table, int table_size) {
    for (int i = 0; i < table_size; ++i) {
        if (table[i].occupied) {
            free(table[i].word);
            free(table[i].lines);
        }
    }
    free(table);
}


// For qsort: compare Entry* by word
int entry_ptr_cmp(const void *a, const void *b) {
    const Entry *ea = *(const Entry **)a;
    const Entry *eb = *(const Entry **)b;
    return strcmp(ea->word, eb->word);
}


int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_file> <index_output_file> <table_size>\n", argv[0]);
        return 1;
    }
    const char *input_file = argv[1];
    const char *output_file = argv[2];
    int table_size = atoi(argv[3]);
    if (table_size <= 0) {
        printf("Invalid table size.\n");
        return 1;
    }
    Entry *table = calloc(table_size, sizeof(Entry));
    if (!table) {
        printf("Failed to allocate hash table.\n");
        return 1;
    }
    int occupied_count = 0;
    clock_t t0 = clock();
    FILE *fin = fopen(input_file, "r");
    if (!fin) {
        printf("Failed to open input file.\n");
        free(table);
        return 1;
    }
    char line[4096];
    int lineno = 0;
    char *delims = " \t\n,;:.";
    while (fgets(line, sizeof(line), fin)) {
        lineno++;
        // Track words seen on this line (simple array, up to 256 unique words per line)
        char *seen_words[256];
        int seen_count = 0;
        char *token = strtok(line, delims);
        while (token) {
            int already = 0;
            for (int i = 0; i < seen_count; ++i) {
                if (strcmp(seen_words[i], token) == 0) {
                    already = 1;
                    break;
                }
            }
            if (!already) {
                if (insert_word(table, table_size, token, lineno, &occupied_count) < 0) {
                    printf("Memory allocation failed during insertion.\n");
                    fclose(fin);
                    free_table(table, table_size);
                    return 1;
                }
                if (seen_count < 256) {
                    seen_words[seen_count++] = token;
                }
            }
            token = strtok(NULL, delims);
        }
    }
    fclose(fin);
    double load_factor = (double)occupied_count / table_size;
    if (load_factor > LOAD_FACTOR_THRESHOLD) {
        printf("Warning: Table occupancy %.2f exceeds %.2f.\n", load_factor, LOAD_FACTOR_THRESHOLD);
    }
    clock_t t1 = clock();
    double elapsed = (double)(t1 - t0) / CLOCKS_PER_SEC;
    printf("Indexing time: %.6f seconds\n", elapsed);
    // Gather entries for sorting
    Entry **entries = malloc(occupied_count * sizeof(Entry*));
    if (!entries) {
        printf("Failed to allocate sort array.\n");
        free_table(table, table_size);
        return 1;
    }
    int idx = 0;
    for (int i = 0; i < table_size; ++i) {
        if (table[i].occupied) {
            entries[idx++] = &table[i];
        }
    }
    qsort(entries, occupied_count, sizeof(Entry*), entry_ptr_cmp);
    // Write output
    FILE *fout = fopen(output_file, "w");
    if (!fout) {
        printf("Failed to open output file.\n");
        free(entries);
        free_table(table, table_size);
        return 1;
    }
    for (int i = 0; i < occupied_count; ++i) {
        Entry *e = entries[i];
        fprintf(fout, "%s: ", e->word);
        for (int j = 0; j < e->lines_count; ++j) {
            fprintf(fout, "%d%s", e->lines[j], (j + 1 < e->lines_count) ? ", " : "");
        }
        fprintf(fout, "\n");
    }
    fclose(fout);
    free(entries);
    free_table(table, table_size);
    return 0;
}
