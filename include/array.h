// Author: Alex Taradov, https://github.com/ataradov (modified slightly)

#ifndef ARRAY_H
#define ARRAY_H

#include "utilities.h"
#include "allocator.h"

#define define_array(name, array_type, item_type)                                                                   \
                                                                                                                    \
typedef struct array_type {                                                                                         \
    item_type* items;                                                                                               \
    int count;                                                                                                      \
    int capacity;                                                                                                   \
} array_type;                                                                                                       \
                                                                                                                    \
static inline void name##_resize(array_type* array, int capacity) {                                                 \
    array->items = reallocate(array->items, capacity * sizeof(item_type));                                          \
    array->capacity = capacity;                                                                                     \
}                                                                                                                   \
                                                                                                                    \
static inline void name##_extend(array_type* array, int capacity) {                                                 \
    if (array->capacity < capacity) {                                                                               \
        name##_resize(array, capacity * 2);                                                                         \
    }                                                                                                               \
}                                                                                                                   \
                                                                                                                    \
static inline array_type* name##_allocate(int capacity) {                                                           \
    array_type* array = allocate(sizeof(array_type));                                                               \
    array->count = 0;                                                                                               \
    array->capacity = 0;                                                                                            \
    name##_resize(array, capacity);                                                                                 \
    return array;                                                                                                   \
}                                                                                                                   \
                                                                                                                    \
static inline void name##_init(array_type* array, int capacity) {                                                   \
    array->count = 0;                                                                                               \
    array->capacity = 0;                                                                                            \
    name##_resize(array, capacity);                                                                                 \
}                                                                                                                   \
                                                                                                                    \
static inline void name##_free(array_type* array) {                                                                 \
    free(array->items);                                                                                             \
    free(array);                                                                                                    \
}                                                                                                                   \
                                                                                                                    \
static inline void name##_clear(array_type* array) {                                                                \
    array->count = 0;                                                                                               \
}                                                                                                                   \
                                                                                                                    \
static inline int name##_append(array_type* array, item_type item) {                                                \
    name##_extend(array, array->count + 1);                                                                         \
    array->items[array->count++] = item;                                                                            \
    return array->count - 1;                                                                                        \
}                                                                                                                   \
                                                                                                                    \
static inline int name##_append_nocopy(array_type* array) {                                                         \
    name##_extend(array, array->count + 1);                                                                         \
    array->count++;                                                                                                 \
    return array->count - 1;                                                                                        \
}                                                                                                                   \
                                                                                                                    \
static inline void name##_insert(array_type* array, item_type item, int index) {                                    \
    name##_extend(array, array->count + 1);                                                                         \
                                                                                                                    \
    if (index < array->count) {                                                                                     \
        memory_move(array->items + index, array->items + index + 1, (array->capacity - index) * sizeof(item_type)); \
    }                                                                                                               \
                                                                                                                    \
    array->items[index] = item;                                                                                     \
    array->count++;                                                                                                 \
}                                                                                                                   \
                                                                                                                    \
static inline void name##_remove(array_type* array, int index) {                                                    \
    memory_move(array->items + index + 1, array->items + index, (array->capacity - index) * sizeof(item_type));     \
    array->count--;                                                                                                 \
}                                                                                                                   \

#endif
