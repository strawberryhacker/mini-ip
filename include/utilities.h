// Author: strawberryhacker

#ifndef UTILITIES_H
#define UTILITIES_H

#include "stdint.h"
#include "stdbool.h"
#include "stdarg.h"
#include "stdalign.h"
#include "stddef.h"

//--------------------------------------------------------------------------------------------------

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uintptr_t ptr;

//--------------------------------------------------------------------------------------------------

#define _rw volatile
#define __w volatile
#define __r volatile const

#define PACKED __attribute__((__packed__))

#define limit(number, max) (((number > max) ? max : number))

//--------------------------------------------------------------------------------------------------

int format_string(const char* string, char* buffer, int buffer_size, va_list arguments);
void memory_clear(void* memory, int size);
void memory_fill(void* memory, u8 fill, int size);
void memory_copy(const void* source, void* destination, int size);
void memory_move(const void* source, void* destination, int size);
int align_down(int value, int alignment);
int align_up(int value, int alignment);
void* pointer_align_down(void* pointer, int alignment);
void* pointer_align_up(void* pointer, int alignment);

#endif
