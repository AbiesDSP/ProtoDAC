#include "list.h"
#include <stdlib.h>

void list_init(list *self)
{
    self->begin = NULL;
    self->end = NULL;
    self->size = 0;
}

void list_destroy(list *self)
{
    list_node *it = self->begin;
    list_node *next;

    while (it != NULL)
    {
        next = it->next;
        free(it);
        it = next;
    }
}

int list_insert(list *self, const void *ref, int n)
{
    // Allocate a node
    list_node *node = malloc(sizeof(*node));
    node->ref = ref;
    node->next = NULL;
    node->prev = NULL;

    // Size == 0
    if (self->begin == NULL)
    {
        self->begin = node;
        self->end = node;
    }
    else
    {
        // Find the position where we want to insert.
        list_node *pos = list_iter(self, n);
        if (n >= 0)
        {
            node->next = pos;
            node->prev = pos->prev;
            if (node->prev)
            {
                node->prev->next = node;
            }
            else
            {
                self->begin = node;
            }

            pos->prev = node;
        }
        // If negative, append after
        else
        {
            node->next = pos->next;
            node->prev = pos;
            if (node->next)
            {
                node->next->prev = node;
            }
            else
            {
                self->end = node;
            }
            pos->next = node;
        }
    }
    self->size++;
    
    return 0;
}

list_node *list_iter(const list *self, int n)
{
    list_node *node = n >= 0 ? self->begin : self->end;
    if (n >= 0)
    {
        for (int i = 0; i < n; i++)
        {
            node = node->next;
        }
    }
    else
    {
        for (int i = n; i < -1; i++)
        {
            node = node->prev;
        }
    }
    return node;
}
