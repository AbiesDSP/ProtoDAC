#pragma once

#include <stdint.h>

/*
    Doubly linked list.
*/
typedef struct list_node list_node;
typedef struct list list;

struct list_node
{
    const void *ref;
    list_node *next;
    list_node *prev;
};

struct list
{
    list_node *begin;
    list_node *end;
    int size;
};

void list_init(list *self);
void list_destroy(list *self);

list_node *list_iter(const list *self, int n);
int list_insert(list *self, const void *ref, int n);

static inline int list_append(list *self, const void *ref)
{
    return list_insert(self, ref, -1);
}

static inline int list_prepend(list *self, const void *ref)
{
    return list_insert(self, ref, 0);
}

int list_extend(list *self, const list *other);

static inline const void *list_index(const list *self, int n)
{
    return list_iter(self, n)->ref;
}
