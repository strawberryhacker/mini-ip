// Author: strawberryhacker

#ifndef LIST_H
#define LIST_H

//--------------------------------------------------------------------------------------------------

#include "utilities.h"

//--------------------------------------------------------------------------------------------------

typedef struct List List;
typedef struct List ListNode;

struct List {
    List* next;
    List* previous;
};

//--------------------------------------------------------------------------------------------------

#define list_get_struct(node, type, member) \
    (type *)((u8 *)node - offsetof(type, member))

#define list_get_first(list) \
    ((list)->next)

#define list_get_last(list) \
    ((list)->previous)

#define list_iterate_with(node, list) \
    for (node = (list)->next; node != (list); node = node->next)

#define list_iterate_reverse_with(node, list) \
    for (node = (list)->previous; node != (list); node = node->previous)

#define list_iterate(node, list) \
    for (ListNode* node = (list)->next; node != (list); node = node->next)

#define list_iterate_reverse(node, list) \
    for (ListNode* node = (list)->previous; node != (list); node = node->previous)

#define list_iterate_safe(node, list) \
    for (ListNode* node = (list)->next, *__##node##_next = node->next; node != (list); \
        node = __##node##_next, __##node##_next = node->next)

#define list_iterate_reverse_safe(node, list) \
    for (ListNode* node = (list)->previous, *__##node##_previous = node->previous; node != (list); \
        node = __##node##_previous, __##node##_previous = node->previous)

//--------------------------------------------------------------------------------------------------

static inline void list_init(List* list) {
    list->previous = list;
    list->next = list;
}

//--------------------------------------------------------------------------------------------------

static inline void list_add_first(ListNode* node, List* list) {
    node->next = list->next;
    node->previous = list;
    list->next->previous = node;
    list->next = node;
}

//--------------------------------------------------------------------------------------------------

static inline void list_add_last(ListNode* node, List* list) {
    node->next = list;
    node->previous = list->previous;
    list->previous->next = node;
    list->previous = node;
}

//--------------------------------------------------------------------------------------------------

static inline void list_add_before(ListNode* node, ListNode* before) {
    node->next = before->next;
    node->previous = before;
    before->next->previous = node;
    before->next = node;
}

//--------------------------------------------------------------------------------------------------

static inline void list_remove(ListNode* node) {
    node->next->previous = node->previous;
    node->previous->next = node->next;
}

//--------------------------------------------------------------------------------------------------

static inline List* list_remove_first(List* list) {
    if (list->next == list) {
        return 0;
    }

    List* node = list->next;
    list_remove(list->next);
    return node;
}

//--------------------------------------------------------------------------------------------------

static inline List* list_remove_last(List* list) {
    if (list->previous == list) {
        return 0;
    }

    List* node = list->previous;
    list_remove(list->previous);
    return node;
}

//--------------------------------------------------------------------------------------------------

static inline bool list_is_empty(List* list) {
    return list->next == list;
}

//--------------------------------------------------------------------------------------------------

static inline int list_get_size(List* list) {
    int size = 0;

    list_iterate(it, list) {
        size++;
    }

    return size;
}

//--------------------------------------------------------------------------------------------------

static inline void list_merge(List* source, List* destination) {
    source->previous->next = destination->next;
    destination->next->previous = source->previous;
    destination->next = source->next;
    destination->next->previous = destination;
}

#endif
