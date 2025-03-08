/**
 * @file map.h
 * @brief A generic hashmap implementation for key-value storage
 */

#ifndef MAP_H
#define MAP_H

#include <stdint.h>
#include <sys/types.h>

/**
 * @brief Maximum allowed length for keys
 */
#define MAP_KEY_MAX_LEN (128)

/**
 * @brief Opaque map structure
 */
typedef struct map map_t;

/**
 * @brief Opaque map iterator structure
 */
typedef struct map_iter map_iter_t;

/**
 * @brief Creates a new map
 *
 * @param size Size in bytes of the values to be stored
 * @return Pointer to the newly created map, or NULL on failure
 */
map_t* map_create(size_t size);

/**
 * @brief Inserts a key-value pair into the map
 *
 * @param this Pointer to the map
 * @param key The key string
 * @param len Length of the key (including null terminator if needed)
 * @param element Pointer to the value to be stored
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid parameters
 *         -EEXIST: Key already exists
 *         -ENOMEM: Memory allocation failed
 *         -EOVERFLOW: Key too long
 */
ssize_t map_put(map_t* this, const char* key, size_t len, void* element);

/**
 * @brief Retrieves a value from the map
 *
 * @param this Pointer to the map
 * @param key The key string
 * @param len Length of the key (including null terminator if needed)
 * @param out Pointer where the value will be stored
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid parameters
 *         -ENOENT: Key not found
 */
ssize_t map_get(map_t* this, const char* key, size_t len, void* out);

/**
 * @brief Removes a key-value pair from the map
 *
 * @param this Pointer to the map
 * @param key The key string
 * @param len Length of the key (including null terminator if needed)
 * @param out Pointer where the removed value will be stored
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid parameters
 *         -ENOENT: Key not found
 */
ssize_t map_remove(map_t* this, const char* key, size_t len, void* out);

/**
 * @brief Returns the number of key-value pairs in the map
 *
 * @param this Pointer to the map
 * @return Number of entries in the map
 */
size_t map_count(const map_t* this);

/**
 * @brief Frees all memory associated with the map
 *
 * @param this Pointer to the map
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid parameter
 */
ssize_t map_free(map_t* this);

/**
 * @brief Creates an iterator for the map
 *
 * @param map Pointer to the map
 * @return Pointer to the newly created iterator, or NULL on failure
 */
map_iter_t* map_iter_create(map_t* map);

/**
 * @brief Gets the next key-value pair from the iterator
 *
 * @param iter Pointer to the iterator
 * @param key_out Pointer where the key will be stored
 * @param key_len_out Pointer where the key length will be stored
 * @param value_out Pointer where the value will be stored
 * @return 0 on success, negative error code on failure or end of iteration:
 *         -EINVAL: Invalid parameters
 *         -ENOENT: No more elements
 */
ssize_t map_iter_next(map_iter_t* iter, void* key_out, size_t* key_len_out, void* value_out);

/**
 * @brief Frees the iterator
 *
 * @param iter Pointer to the iterator
 * @return 0 on success, negative error code on failure:
 *         -EINVAL: Invalid parameter
 */
ssize_t map_iter_free(map_iter_t* iter);

#endif /* MAP_H */
