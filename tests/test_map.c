#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#include "map.h"

void setUp(void) {
}

void tearDown(void) {
}

static void test_basic_operations(void) {
    map_t* map = map_create(sizeof(void*));
    TEST_ASSERT_NOT_NULL(map);

    char* value = "HelloWorld";
    char key[]  = "MyKey";
    TEST_ASSERT_EQUAL_INT(0, map_put(map, key, sizeof(key), &value));

    char* ptr = NULL;
    TEST_ASSERT_EQUAL_INT(0, map_get(map, key, sizeof(key), &ptr));
    TEST_ASSERT_EQUAL_PTR(value, ptr);

    // Remove the key
    ptr = NULL;
    TEST_ASSERT_EQUAL_INT(0, map_remove(map, key, sizeof(key), &ptr));
    TEST_ASSERT_EQUAL_PTR(value, ptr);

    // Verify it's gone
    ptr = NULL;
    TEST_ASSERT_EQUAL_INT(-ENOENT, map_get(map, key, sizeof(key), &ptr));

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_multiple_values(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    char key1[] = "key1";
    char key2[] = "key2";
    char key3[] = "key3";

    int val1    = 100;
    int val2    = 200;
    int val3    = 300;

    TEST_ASSERT_EQUAL_INT(0, map_put(map, key1, sizeof(key1), &val1));
    TEST_ASSERT_EQUAL_INT(0, map_put(map, key2, sizeof(key2), &val2));
    TEST_ASSERT_EQUAL_INT(0, map_put(map, key3, sizeof(key3), &val3));

    int result = 0;
    TEST_ASSERT_EQUAL_INT(0, map_get(map, key1, sizeof(key1), &result));
    TEST_ASSERT_EQUAL_INT(val1, result);

    TEST_ASSERT_EQUAL_INT(0, map_get(map, key2, sizeof(key2), &result));
    TEST_ASSERT_EQUAL_INT(val2, result);

    TEST_ASSERT_EQUAL_INT(0, map_get(map, key3, sizeof(key3), &result));
    TEST_ASSERT_EQUAL_INT(val3, result);

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_collision_handling(void) {
    // This test creates a situation where collisions are likely
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    // We'll use keys that might produce the same hash index
    // Even without direct capacity control, with enough keys we'll get collisions

    char* keys[] =
        {"key1", "key2", "key3", "key4", "key5", "key6", "key7", "key8", "key9", "key10"};

    int values[10];
    for (int i = 0; i < 10; i++) {
        values[i] = i * 100;
        TEST_ASSERT_EQUAL_INT(0, map_put(map, keys[i], strlen(keys[i]) + 1, &values[i]));
    }

    // Verify all values can be retrieved correctly despite collisions
    int result = 0;
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL_INT(0, map_get(map, keys[i], strlen(keys[i]) + 1, &result));
        TEST_ASSERT_EQUAL_INT(values[i], result);
    }

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_resize_minimal(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    // Add just enough elements to potentially trigger a resize
    for (int i = 0; i < 40; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int value  = i * 10;
        int result = map_put(map, key, strlen(key) + 1, &value);
        TEST_ASSERT_EQUAL_INT(0, result);

        // Verify we can get it back
        int get_result = 0;
        int get_status = map_get(map, key, strlen(key) + 1, &get_result);
        TEST_ASSERT_EQUAL_INT(0, get_status);
        TEST_ASSERT_EQUAL_INT(value, get_result);
    }

    // Remove just a few elements
    for (int i = 0; i < 10; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int result        = 0;
        int remove_status = map_remove(map, key, strlen(key) + 1, &result);
        TEST_ASSERT_EQUAL_INT(0, remove_status);
    }

    // Verify remaining elements are accessible
    for (int i = 10; i < 40; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int result     = 0;
        int get_status = map_get(map, key, strlen(key) + 1, &result);
        TEST_ASSERT_EQUAL_INT(0, get_status);
        TEST_ASSERT_EQUAL_INT(i * 10, result);
    }

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_resize_behavior(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    // Add enough elements to trigger resize
    for (int i = 0; i < 100; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int value = i * 10;
        TEST_ASSERT_EQUAL_INT(0, map_put(map, key, strlen(key) + 1, &value));
    }

    // Verify all elements are still accessible after resize
    for (int i = 0; i < 100; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int result = 0;
        TEST_ASSERT_EQUAL_INT(0, map_get(map, key, strlen(key) + 1, &result));
        TEST_ASSERT_EQUAL_INT(i * 10, result);
    }

    // Now remove most elements to trigger shrink
    for (int i = 0; i < 80; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int result = 0;
        TEST_ASSERT_EQUAL_INT(0, map_remove(map, key, strlen(key) + 1, &result));
    }

    // Verify removed elements are gone
    for (int i = 0; i < 80; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int result = 0;
        // Should return ENOENT since we removed these keys
        TEST_ASSERT_EQUAL_INT(-ENOENT, map_get(map, key, strlen(key) + 1, &result));
    }

    // Verify remaining elements are still accessible
    for (int i = 80; i < 100; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int result = 0;
        TEST_ASSERT_EQUAL_INT(0, map_get(map, key, strlen(key) + 1, &result));
        TEST_ASSERT_EQUAL_INT(i * 10, result);
    }

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_iterator(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    // Add some elements
    const int num_elements = 10;
    for (int i = 0; i < num_elements; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int value = i * 10;
        TEST_ASSERT_EQUAL_INT(0, map_put(map, key, strlen(key) + 1, &value));
    }

    // Test iteration
    map_iter_t* iter = map_iter_create(map);
    TEST_ASSERT_NOT_NULL(iter);

    int count = 0;
    char* key_ptr;
    size_t key_len;
    int value;

    while (map_iter_next(iter, &key_ptr, &key_len, &value) == 0) {
        // Verify the key format matches what we expect
        TEST_ASSERT_TRUE(strncmp(key_ptr, "key", 3) == 0);

        // Extract the index from the key
        int key_index = atoi(key_ptr + 3);
        TEST_ASSERT_TRUE(key_index >= 0 && key_index < num_elements);

        // Verify value matches what we put in
        TEST_ASSERT_EQUAL_INT(key_index * 10, value);

        count++;
    }

    // Verify we iterated over all elements
    TEST_ASSERT_EQUAL_INT(num_elements, count);

    TEST_ASSERT_EQUAL_INT(0, map_iter_free(iter));
    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_error_cases(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    int value = 123;

    // Test with NULL or invalid inputs
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_put(NULL, "key", 4, &value));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_put(map, NULL, 4, &value));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_put(map, "key", 0, &value));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_put(map, "key", 4, NULL));

    // Test get with invalid inputs
    int result = 0;
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_get(NULL, "key", 4, &result));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_get(map, NULL, 4, &result));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_get(map, "key", 0, &result));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_get(map, "key", 4, NULL));

    // Test remove with invalid inputs
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_remove(NULL, "key", 4, &result));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_remove(map, NULL, 4, &result));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_remove(map, "key", 0, &result));
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_remove(map, "key", 4, NULL));

    // Test duplicate keys
    char key[] = "duplicate";
    TEST_ASSERT_EQUAL_INT(0, map_put(map, key, sizeof(key), &value));
    TEST_ASSERT_EQUAL_INT(-EEXIST, map_put(map, key, sizeof(key), &value));

    // Test nonexistent key
    TEST_ASSERT_EQUAL_INT(-ENOENT, map_get(map, "nonexistent", 11, &result));

    TEST_ASSERT_EQUAL_INT(0, map_free(map));

    // Test free with NULL
    TEST_ASSERT_EQUAL_INT(-EINVAL, map_free(NULL));
}

static void test_key_handling(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    // Test with a single key first
    char key[]     = "testkey";
    int value      = 123;

    int put_result = map_put(map, key, strlen(key) + 1, &value);
    TEST_ASSERT_EQUAL_INT(0, put_result);

    int retrieved  = 0;
    int get_result = map_get(map, key, strlen(key) + 1, &retrieved);
    TEST_ASSERT_EQUAL_INT(0, get_result);
    TEST_ASSERT_EQUAL_INT(value, retrieved);

    // Now try with a series of keys
    for (int i = 0; i < 30; i++) {
        char num_key[20];
        sprintf(num_key, "key%d", i);
        int num_value  = i * 100;

        size_t key_len = strlen(num_key) + 1;
        int num_put    = map_put(map, num_key, key_len, &num_value);
        TEST_ASSERT_EQUAL_INT(0, num_put);

        // Immediately verify we can get it back
        int retrieved_val = 0;
        int num_get       = map_get(map, num_key, key_len, &retrieved_val);
        if (num_get != 0) {
            TEST_FAIL_MESSAGE("Failed to retrieve key immediately after insertion");
        }
        TEST_ASSERT_EQUAL_INT(num_value, retrieved_val);
    }

    // Now verify we can get all keys in reverse order
    for (int i = 29; i >= 0; i--) {
        char num_key[20];
        sprintf(num_key, "key%d", i);
        size_t key_len    = strlen(num_key) + 1;

        int expected_val  = i * 100;
        int retrieved_val = 0;

        int num_get       = map_get(map, num_key, key_len, &retrieved_val);
        TEST_ASSERT_EQUAL_INT(0, num_get);
        TEST_ASSERT_EQUAL_INT(expected_val, retrieved_val);
    }

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

static void test_remove_operations(void) {
    map_t* map = map_create(sizeof(int));
    TEST_ASSERT_NOT_NULL(map);

    // Add a series of keys
    for (int i = 0; i < 20; i++) {
        char key[20];
        sprintf(key, "key%d", i);
        int value      = i * 100;
        int put_result = map_put(map, key, strlen(key) + 1, &value);
        TEST_ASSERT_EQUAL_INT(0, put_result);
    }

    // Now remove every other key
    for (int i = 0; i < 20; i += 2) {
        char key[20];
        sprintf(key, "key%d", i);
        int value         = 0;
        int remove_result = map_remove(map, key, strlen(key) + 1, &value);
        TEST_ASSERT_EQUAL_INT(0, remove_result);
        TEST_ASSERT_EQUAL_INT(i * 100, value);
    }

    // Verify remaining keys are still accessible
    for (int i = 1; i < 20; i += 2) {
        char key[20];
        sprintf(key, "key%d", i);
        int value      = 0;
        int get_result = map_get(map, key, strlen(key) + 1, &value);
        TEST_ASSERT_EQUAL_INT(0, get_result);
        TEST_ASSERT_EQUAL_INT(i * 100, value);
    }

    // Verify removed keys are not accessible
    for (int i = 0; i < 20; i += 2) {
        char key[20];
        sprintf(key, "key%d", i);
        int value      = 0;
        int get_result = map_get(map, key, strlen(key) + 1, &value);
        TEST_ASSERT_EQUAL_INT(-ENOENT, get_result);
    }

    TEST_ASSERT_EQUAL_INT(0, map_free(map));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_key_handling);
    RUN_TEST(test_remove_operations);
    RUN_TEST(test_basic_operations);
    RUN_TEST(test_multiple_values);
    RUN_TEST(test_collision_handling);
    RUN_TEST(test_resize_minimal);
    RUN_TEST(test_resize_behavior);
    RUN_TEST(test_iterator);
    RUN_TEST(test_error_cases);
    return UNITY_END();
}
