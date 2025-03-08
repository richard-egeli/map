# Map Library

A lightweight, efficient key-value map implementation in C.

## Overview

This library provides a simple but powerful hash map implementation for C projects. It offers standard map operations with careful error handling and consistent interfaces.

## Features

- Fast key-value lookups with hash-based implementation
- String-based keys with configurable length
- Fixed-size value storage with type safety
- Comprehensive error reporting through negative errno values
- Memory-efficient implementation
- Iterator support for traversing all map entries

## Constraints

- Maximum key length: 128 bytes (defined as `MAP_KEY_MAX_LEN`)
- Values must be of fixed size (specified at map creation)

## API Reference

### API Overview

### Map Lifecycle

- `map_t* map_create(size_t size)` - Create a new map with fixed-size values
- `ssize_t map_free(map_t* this)` - Free all memory associated with the map

### Basic Operations

- `ssize_t map_put(map_t* this, const char* key, size_t len, void* element)` - Add or update an entry
- `ssize_t map_get(map_t* this, const char* key, size_t len, void* out)` - Retrieve a value
- `ssize_t map_remove(map_t* this, const char* key, size_t len, void* out)` - Remove an entry
- `size_t map_count(const map_t* this)` - Get number of entries

### Iteration

- `map_iter_t* map_iter_create(map_t* map)` - Create an iterator
- `ssize_t map_iter_next(map_iter_t* iter, void* key_out, size_t* key_len_out, void* value_out)` - Get next item
- `ssize_t map_iter_free(map_iter_t* iter)` - Free the iterator

## Error Handling

All functions return either `0` for success or a negative error code:

| Error Code | Description |
|------------|-------------|
| `-EINVAL`  | Invalid parameters (null pointers, etc.) |
| `-ENOENT`  | Key not found |
| `-EEXIST`  | Key already exists (on insertion) |
| `-ENOMEM`  | Memory allocation failed |
| `-EOVERFLOW` | Key too long (> 128 bytes) |


## Usage Example

```c
#include "map.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    int value;
    char description[32];
} my_data_t;

int main() {
    // Create a map for storing my_data_t values
    map_t* my_map = map_create(sizeof(my_data_t));
    if (!my_map) {
        fprintf(stderr, "Failed to create map\n");
        return 1;
    }
    
    // Add some data
    my_data_t data1 = {42, "The answer"};
    ssize_t result = map_put(my_map, "key1", 5, &data1);
    if (result < 0) {
        fprintf(stderr, "Failed to insert: %zd\n", result);
    }
    
    // Retrieve data
    my_data_t retrieved;
    result = map_get(my_map, "key1", 5, &retrieved);
    if (result == 0) {
        printf("Retrieved: %d - %s\n", retrieved.value, retrieved.description);
    }
    
    // Iterate through all elements
    map_iter_t* iter = map_iter_create(my_map);
    if (iter) {
        char key[MAP_KEY_MAX_LEN];
        size_t key_len;
        my_data_t value;
        
        while (map_iter_next(iter, key, &key_len, &value) == 0) {
            printf("Key: %s, Value: %d - %s\n", key, value.value, value.description);
        }
        
        map_iter_free(iter);
    }
    
    // Clean up
    map_free(my_map);
    return 0;
}
```

## Integration

### Using as a Git Submodule

```bash
git submodule add https://github.com/richard-egeli/map.git external/map
```

### CMake Integration

```cmake
# Method 1: Add as a subdirectory
add_subdirectory(external/map)
target_link_libraries(your_target PRIVATE map)

# Method 2: Using as an OBJECT library to prevent multiple linking
add_library(map_obj OBJECT
    ${MAP_SOURCES}
    ${MAP_HEADERS}
)
target_include_directories(map_obj PUBLIC ${MAP_INCLUDE_DIRS})
target_link_libraries(your_target PRIVATE $<TARGET_OBJECTS:map_obj>)
```

## Preventing Multiple Linkage

When using this library with submodules in multiple projects, you can prevent duplicate linking using:

1. **CMake OBJECT Libraries**: Create the library once as an OBJECT library and use `$<TARGET_OBJECTS:map_obj>` in your targets
2. **Interface Libraries**: Make your map library an INTERFACE library if it's header-only
3. **Import Targets**: Use imported targets with clear target names to enable CMake's built-in target de-duplication

```cmake
# Example to prevent duplicate linking
if(NOT TARGET map)
    add_subdirectory(external/map)
endif()
```
