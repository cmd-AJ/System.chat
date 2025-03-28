#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include <ctype.h>
#include <time.h>

// Hash function to calculate an index from the key
unsigned int hash(const char *key) {
    unsigned int hash_value = 0;
    while (*key) {
        hash_value = (hash_value << 5) + *key++;
    }
    return hash_value % TABLE_SIZE;
}

// Create a new hash table entry (key-value pair)
struct hash_entry* create_entry(const char *key, const char *value) {
    struct hash_entry *entry = (struct hash_entry *)malloc(sizeof(struct hash_entry));
    entry->key = strdup(key);
    entry->value = strdup(value);
    entry->next = NULL;
    return entry;
}

// Insert a key-value pair into the hash table
void insert(struct hash_table *table, const char *key, const char *value) {
    unsigned int index = hash(key);
    struct hash_entry *entry = create_entry(key, value);

    // Insert the entry at the beginning of the bucket list
    entry->next = table->buckets[index];
    table->buckets[index] = entry;
}

// Retrieve a value by key from the hash table
char* get(struct hash_table *table, const char *key) {
    unsigned int index = hash(key);
    struct hash_entry *entry = table->buckets[index];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL; // Key not found
}


void remove_quotes(char *str) {
    if (str[0] == '"' && str[strlen(str) - 1] == '"') {
        // Remove the closing quote
        str[strlen(str) - 1] = '\0';
        // Shift the string left by one to remove the opening quote
        memmove(str, str + 1, strlen(str));
    }
}

// Creditos al mr de Stephen y Cplus
// https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trim(char *str)
{
    size_t len = 0;
    char *frontp = str;
    char *endp = NULL;
    
    if( str == NULL ) { return NULL; }
    if( str[0] == '\0' ) { return str; }
    
    len = strlen(str);
    endp = str + len;
    
    /* Move the front and back pointers to address the first non-whitespace
     * characters from each end.
     */
    while( isspace((unsigned char) *frontp) ) { ++frontp; }
    if( endp != frontp )
    {
        while( isspace((unsigned char) *(--endp)) && endp != frontp ) {}
    }
    
    if(frontp != str && endp == frontp )
    {
        // Empty string
        *(isspace((unsigned char) *endp) ? str : (endp + 1)) = '\0';
    }
    else if( str + len - 1 != endp )
            *(endp + 1) = '\0';
    
    /* Shift the string so that it starts at str so that if it's dynamically
     * allocated, we can still free it on the returned pointer.  Note the reuse
     * of endp to mean the front of the string buffer now.
     */
    endp = str;
    if( frontp != str )
    {
            while( *frontp ) { *endp++ = *frontp++; }
            *endp = '\0';
    }
    
    return str;
}

void clean_json_message(char *message) {
    size_t len = strlen(message);

    if (len < 2) {  // Must be at least "{}"
        message[0] = '\0';  // Empty string
        return;
    }

    // Shift characters left to remove the first '{'
    memmove(message, message + 1, len - 1);

    // Find the last '}' and replace it with '\0'
    char *last_brace = strrchr(message, '}');  // Find last '}'
    if (last_brace) {
        *last_brace = '\0';  // Remove it
    }
}


void gettime(char *buffer, size_t buffer_size) {
    time_t now;
    struct tm *timeinfo;

    time(&now);
    timeinfo = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

