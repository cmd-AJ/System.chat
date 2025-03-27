#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#define TABLE_SIZE 10

// Hash entry (key-value pair)
struct hash_entry {
    char *key;
    char *value;
    struct hash_entry *next;
};

// Hash table structure
struct hash_table {
    struct hash_entry *buckets[TABLE_SIZE];
};

// Function prototypes
unsigned int hash(const char *key);
struct hash_entry* create_entry(const char *key, const char *value);
void insert(struct hash_table *table, const char *key, const char *value);
char* get(struct hash_table *table, const char *key);
void free_table(struct hash_table *table);
void remove_quotes(char *str);
char* trim(char *str);
void clean_json_message(char *message);
void gettime(char *buffer, size_t buffer_size) ;

#endif // HASH_TABLE_H
