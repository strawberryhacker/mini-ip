// Author: strawberryhacker

#include "allocator.h"

//--------------------------------------------------------------------------------------------------

#define ALIGNMENT 8

//--------------------------------------------------------------------------------------------------

typedef struct Block Block;
struct Block {
    Block*  next;
    int     size;
};

typedef struct {
    Block   dummy_block;
    Block*  dummy;
    Block*  last;
    int     capacity;
    int     used;
} Allocator;

//--------------------------------------------------------------------------------------------------

static Allocator allocator;

//--------------------------------------------------------------------------------------------------

void allocator_set_memory_region(u8* memory, int size) {
    u8* memory_start = pointer_align_up(memory, ALIGNMENT);
    u8* memory_end = pointer_align_down(memory + size, ALIGNMENT);

    memory_end -= sizeof(Block);

    allocator.capacity = memory_end - memory_start;
    allocator.used = 0;

    Block* start = (Block *)memory_start;
    Block* end   = (Block *)memory_end;

    start->size = allocator.capacity;
    start->next = end;
    
    end->size = 0;
    end->next = 0;

    allocator.dummy = &allocator.dummy_block;
    allocator.dummy->size = 0;
    allocator.dummy->next = start;
    allocator.last = end;
}

//--------------------------------------------------------------------------------------------------

int allocator_get_capacity() {
    return allocator.capacity;
}

//--------------------------------------------------------------------------------------------------

int allocator_get_used() {
    return allocator.used;
}

//--------------------------------------------------------------------------------------------------

static void insert_block(Block* block) {
    Block* it;
    for (it = allocator.dummy; it && (ptr)it->next <= (ptr)block; it = it->next);

    // Check for backward coalescing.
    if ((ptr)it + it->size == (ptr)block) {
        it->size += block->size;
        block = it;
    }

    // Check for forward coalescing.
    if ((ptr)block + block->size == (ptr)block && it->next != allocator.last) {
        block->size += it->next->size;
        block->next = it->next->next;
    }
    else {
        block->next = it->next;
    }

    // Fix the pointers in case of backward coalescing.
    if (block != it) {
        it->next = block;
    }
}

//--------------------------------------------------------------------------------------------------

void* allocate(int size) {
    size = align_up(size + sizeof(Block), ALIGNMENT);

    Block* previous = allocator.dummy;
    Block* current = allocator.dummy->next;

    while (current && current->size < size) {
        previous = current;
        current = current->next;
    }

    if (current == 0) {
        return 0;
    }

    previous->next = current->next;

    // Split the current block if it can contain the current allocation plus a new one.
    if (current->size - size >= sizeof(Block) + ALIGNMENT) {
        Block* block = (Block *)((u8 *)current + size);
        block->size = current->size - size;
        current->size = size;
        insert_block(block);
    }

    allocator.used += current->size;
    return current + 1;
}

//--------------------------------------------------------------------------------------------------

void* reallocate(void* old, int size) {
    void* new = allocate(size);

    if (old == 0) {
        return new;
    }
    
    Block* old_block = (Block *)old - 1;
    memory_copy(old, new, old_block->size);
    free(old);
    return new;
}

//--------------------------------------------------------------------------------------------------

void free(void* pointer) {
    Block* block = (Block *)pointer - 1;
    allocator.used -= block->size;
    insert_block(block);
}
