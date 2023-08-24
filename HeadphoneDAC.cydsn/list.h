#pragma once

#include <stdint.h>

/*
    Doubly linked List.
*/
typedef struct ListNode ListNode;
typedef struct List List;

struct ListNode
{
    void *ref;
    ListNode *next;
    ListNode *prev;
};

struct List
{
    ListNode *begin;
    ListNode *end;
    int size;
};

void list_init(List *self);
void list_destroy(List *self);

ListNode *list_iter(const List *self, int n);
int list_insert(List *self, const void *ref, int n);

static inline int list_append(List *self, const void *ref)
{
    return list_insert(self, ref, -1);
}

static inline int list_prepend(List *self, const void *ref)
{
    return list_insert(self, ref, 0);
}

int list_extend(List *self, const List *other);

static inline void *list_index(const List *self, int n)
{
    return list_iter(self, n)->ref;
}
