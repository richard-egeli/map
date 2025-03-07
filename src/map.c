#include "map.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct string {
    size_t size;
    char bytes[];
} string_t;

typedef struct map_kv {
    struct map_kv* next;
    uint32_t hash;
    string_t* key;
    uint8_t value[];
} map_kv_t;

typedef struct map {
    map_kv_t** elements;
    size_t capacity;
    size_t count;
    size_t size;
} map_t;

typedef struct map_iter {
    map_t* map;
    size_t index;
    map_kv_t* current;
} map_iter_t;

static const size_t precomputed_prime_table[] = {
    31,
    67,
    137,
    277,
    557,
    1117,
    2237,
    4481,
    8963,
    17929,
    35863,
    71741,
    143483,
    286973,
    573953,
    1147921,
};

static inline uint32_t murmur_hash2(const char* str, size_t len) {
    uint32_t h     = 0;
    const size_t m = 0x5bd1e995;
    const int r    = 24;

    while (len >= 4) {
        uint32_t k = ((uint32_t)str[0]) | ((uint32_t)str[1] << 8) | ((uint32_t)str[2] << 16) |
                     ((uint32_t)str[3] << 24);

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        str += 4;
        len -= 4;
    }

    switch (len) {
        case 3:
            h ^= (uint32_t)str[2] << 16;
        case 2:
            h ^= (uint32_t)str[1] << 8;
        case 1:
            h ^= (uint32_t)str[0];
            h *= m;
        default:
            break;
    }

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return h;
}

static inline ssize_t map_next_prime_size(const map_t* map) {
    if (map == NULL) {
        return -EINVAL;
    }

    const size_t count = sizeof(precomputed_prime_table) / sizeof(*precomputed_prime_table);
    size_t result      = map->capacity;

    for (size_t i = 0; i < count; i++) {
        const size_t tmp = precomputed_prime_table[i];
        if (tmp > result) {
            result = tmp;
            break;
        }
    }

    if (result >= SSIZE_MAX) {
        return -EOVERFLOW;
    }

    if (result == map->capacity) {
        errno = ERANGE;
        return -1;
    }

    return (ssize_t)result;
}

static inline ssize_t map_prev_prime_size(const map_t* map) {
    if (map == NULL) {
        return -EINVAL;
    }

    const size_t count = sizeof(precomputed_prime_table) / sizeof(*precomputed_prime_table);

    if (map->capacity <= precomputed_prime_table[0]) {
        return -ERANGE;
    }

    size_t result = precomputed_prime_table[0];
    for (size_t i = 0; i < count; i++) {
        const size_t tmp = precomputed_prime_table[i];
        if (tmp >= map->capacity) {
            break;
        }

        result = tmp;
    }

    if (result == map->capacity) {
        return -ERANGE;
    }

    return (ssize_t)result;
}

static ssize_t map_resize(map_t* map, size_t new_capacity) {
    if (map == NULL || map->elements == NULL || new_capacity == 0) {
        return -EINVAL;
    }

    // Allocate new elements array
    map_kv_t** new_elements = calloc(new_capacity, sizeof(map_kv_t*));
    if (new_elements == NULL) {
        return -ENOMEM;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        map_kv_t* current = map->elements[i];

        while (current != NULL) {
            map_kv_t* next          = current->next;
            uint32_t new_index      = current->hash % new_capacity;

            current->next           = new_elements[new_index];
            new_elements[new_index] = current;
            current                 = next;
        }
    }

    // Free old elements array and update map
    free(map->elements);
    map->elements = new_elements;
    map->capacity = new_capacity;

    return 0;
}

ssize_t map_put(map_t* map, const char* key, size_t size, void* element) {
    if (map == NULL || key == NULL || size == 0 || element == NULL) {
        return -EINVAL;
    }

    if (size > MAP_KEY_MAX_LEN) {
        return -EOVERFLOW;
    }

    if (map->count >= (map->capacity * 3) / 4) {
        ssize_t new_capacity = map_next_prime_size(map);
        if (new_capacity < 0) {
            return new_capacity;
        }

        ssize_t resize_result = map_resize(map, (size_t)new_capacity);
        if (resize_result < 0) {
            return resize_result;
        }
    }

    uint32_t hash     = murmur_hash2(key, size);
    uint32_t index    = hash % map->capacity;
    map_kv_t* current = map->elements[index];

    // Check for existing entry with the key
    while (current != NULL) {
        if (current->key->size == size && strncmp(current->key->bytes, key, size) == 0) {
            return -EEXIST;
        }

        current = current->next;
    }

    // Allocate bucket with space for the flexible array member
    map_kv_t* bck = malloc(sizeof(map_kv_t) + map->size);
    if (bck == NULL) {
        return -ENOMEM;
    }

    string_t* str = malloc(sizeof(string_t) + size + 1);
    if (str == NULL) {
        free(bck);
        return -ENOMEM;
    }

    memcpy(str->bytes, key, size);
    str->bytes[size] = '\0';
    str->size        = size;
    bck->hash        = hash;
    bck->key         = str;
    bck->next        = map->elements[index];

    // Copy the element data into the flexible array member
    memcpy(bck->value, element, map->size);

    map->elements[index] = bck;
    map->count++;

    return 0;
}

ssize_t map_get(map_t* map, const char* key, size_t size, void* out) {
    if (map == NULL || key == NULL || size == 0 || out == NULL) {
        return -EINVAL;
    }

    uint32_t hash     = murmur_hash2(key, size);
    uint32_t index    = hash % map->capacity;
    map_kv_t* element = map->elements[index];

    while (element != NULL) {
        if (element->key->size == size && strncmp(element->key->bytes, key, size) == 0) {
            memcpy(out, element->value, map->size);
            return 0;
        }

        element = element->next;
    }

    return -ENOENT;
}

ssize_t map_remove(map_t* map, const char* key, size_t len, void* out) {
    if (map == NULL || key == NULL || len == 0 || out == NULL) {
        return -EINVAL;
    }

    uint32_t hash      = murmur_hash2(key, len);
    uint32_t index     = hash % map->capacity;
    map_kv_t* current  = map->elements[index];
    map_kv_t* previous = NULL;

    while (current != NULL) {
        if (current->key->size == len && strncmp(current->key->bytes, key, len) == 0) {
            memcpy(out, current->value, map->size);

            if (previous == NULL) {
                map->elements[index] = current->next;
            } else {
                previous->next = current->next;
            }

            free(current->key);
            free(current);
            map->count--;

            if (map->count < map->capacity / 4 && map->capacity > precomputed_prime_table[0]) {
                ssize_t new_capacity = map_prev_prime_size(map);
                if (new_capacity > 0) {
                    map_resize(map, (size_t)new_capacity);
                }
            }

            return 0;
        }

        previous = current;
        current  = current->next;
    }

    return -ENOENT;
}

ssize_t map_free(map_t* map) {
    if (map == NULL || map->elements == NULL) {
        return -EINVAL;
    }

    for (size_t i = 0; i < map->capacity; i++) {
        map_kv_t* current = map->elements[i];
        while (current != NULL) {
            map_kv_t* next = current->next;
            free(current->key);
            free(current);
            current = next;
        }
    }

    free(map->elements);
    free(map);
    return 0;
}

map_t* map_create(size_t size) {
    map_t* map = malloc(sizeof(*map));
    if (map == NULL) {
        return NULL;
    }

    map->elements = calloc(precomputed_prime_table[0], sizeof(map_kv_t*));
    if (map->elements == NULL) {
        return NULL;
    }

    map->capacity = precomputed_prime_table[0];
    map->size     = size;
    map->count    = 0;
    return map;
}

ssize_t map_iter_next(map_iter_t* iter, void* key_out, size_t* key_len_out, void* value_out) {
    if (iter == NULL || key_out == NULL || key_len_out == NULL || value_out == NULL) {
        return -EINVAL;
    }

    // Check if we're at the end of the map
    if (iter->index >= iter->map->capacity) {
        return -ENOENT;
    }

    // Copy current element's data
    char** key_out_ptr = (char**)key_out;
    *key_out_ptr       = iter->current->key->bytes;
    *key_len_out       = iter->current->key->size;
    memcpy(value_out, iter->current->value, iter->map->size);

    // Move to the next element
    if (iter->current->next != NULL) {
        // Move to the next element in the current bucket
        iter->current = iter->current->next;
    } else {
        // Move to the next non-empty bucket
        iter->current = NULL;
        iter->index++;

        while (iter->index < iter->map->capacity && iter->map->elements[iter->index] == NULL) {
            iter->index++;
        }

        if (iter->index < iter->map->capacity) {
            iter->current = iter->map->elements[iter->index];
        }
    }

    return 0;
}

ssize_t map_iter_free(map_iter_t* iter) {
    if (iter == NULL) {
        return -EINVAL;
    }

    free(iter);
    return 0;
}

map_iter_t* map_iter_create(map_t* map) {
    if (map == NULL) {
        return NULL;
    }

    map_iter_t* iter = malloc(sizeof(*iter));
    if (iter == NULL) {
        return NULL;
    }

    iter->map     = map;
    iter->index   = 0;
    iter->current = NULL;

    // Find the first non-empty bucket
    while (iter->index < map->capacity && map->elements[iter->index] == NULL) {
        iter->index++;
    }

    // Set current to the first element if one exists
    if (iter->index < map->capacity) {
        iter->current = map->elements[iter->index];
    }

    return iter;
}
