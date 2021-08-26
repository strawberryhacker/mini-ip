// Author: strawberryhacker

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include "utilities.h"

//--------------------------------------------------------------------------------------------------

void allocator_set_memory_region(u8* memory, int size);
int allocator_get_capacity();
int allocator_get_used();
void* allocate(int size);
void* reallocate(void* old, int new_size);
void free(void* pointer);

#endif
