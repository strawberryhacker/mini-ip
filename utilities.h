// Author: strawberryhacker

#ifndef UTILITIES_H
#define UTILITIES_H

#include "stdarg.h"
#include "stdbool.h"
#include "stdint.h"

//--------------------------------------------------------------------------------------------------

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float  f32;
typedef double f64;

//--------------------------------------------------------------------------------------------------

#define _rw volatile
#define __w volatile
#define __r volatile const

#define limit(value, max) ((value) < (max)) ? (value) : (max)

#define SECTION(x)    __attribute__((section(x)))
#define PACKED        __attribute__((packed))
#define RAM_FUNCTION  __attribute__((noinline, long_call, section(".ramfunc")))

//--------------------------------------------------------------------------------------------------

static inline void memory_copy(const void* source, void* dest, int size) {
    for (int i = 0; i < size; i++) {
        ((u8 *)dest)[i] = ((const u8 *)source)[i];
    }
}

//--------------------------------------------------------------------------------------------------

static inline void memory_fill(void* dest, u8 fill, int size) {
    for (int i = 0; i < size; i++) {
        ((u8 *)dest)[i] = fill;
    }
}

//--------------------------------------------------------------------------------------------------

static inline bool memory_compare(const void* source1, const void* source2, int size) {
    for (int i = 0; i < size; i++) {
        if (((u8 *)source1)[i] != ((u8 *)source2)[i]) {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------

static inline void memory_move(const void* source, void* dest, int size) {
    const u8* from = source;
    u8* to = dest;

    if (to == from) {
        return;
    }

    if (to > from) {
        for (int i = size; i >= 0; i--) {
            to[i] = from[i];
        }
    }
    else {
        for (int i = 0; i < size; i++) {
            to[i] = from[i];
        }
    }
}

//--------------------------------------------------------------------------------------------------

static inline u32 read_be32(const void* pointer) {
    const u8* source = pointer;
    return source[0] << 24 | source[1] << 16 | source[2] << 8 | source[3];
}

//--------------------------------------------------------------------------------------------------

static inline u16 read_be16(const void* pointer) {
    const u8* source = pointer;
    return source[0] << 8 | source[1];
}

//--------------------------------------------------------------------------------------------------

static inline void write_be32(u32 value, void* pointer) {
    u8* dest = pointer;
    dest[0] = (value >> 24) & 0xFF;
    dest[1] = (value >> 16) & 0xFF;
    dest[2] = (value >> 8 ) & 0xFF;
    dest[3] = (value >> 0 ) & 0xFF;
}

//--------------------------------------------------------------------------------------------------

static inline void write_be64(u64 value, void* pointer) {
    u8* dest = pointer;
    dest[0] = (value >> 56) & 0xFF;
    dest[1] = (value >> 48) & 0xFF;
    dest[2] = (value >> 40) & 0xFF;
    dest[3] = (value >> 32) & 0xFF;
    dest[4] = (value >> 24) & 0xFF;
    dest[5] = (value >> 16) & 0xFF;
    dest[6] = (value >> 8 ) & 0xFF;
    dest[7] = (value >> 0 ) & 0xFF;
}

//--------------------------------------------------------------------------------------------------

static inline void write_be16(u16 value, void* pointer) {
    u8* dest = pointer;
    dest[0] = (value >> 8) & 0xFF;
    dest[1] = (value >> 0) & 0xFF;
}

//--------------------------------------------------------------------------------------------------

static inline int align_up(int value, int alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

//--------------------------------------------------------------------------------------------------

int format_string(const char* string, char* buffer, int buffer_size, va_list arguments);

#endif
