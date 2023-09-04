#pragma once
#include <stdint.h>
#include <string.h>

typedef struct Cptr
{
    uint8_t *ptr;
    uint8_t *begin;
    const uint8_t *end;
} Cptr;

static inline void cptr_init(Cptr *ptr, uint8_t *buf, int size)
{
    ptr->ptr = buf;
    ptr->begin = buf;
    ptr->end = ptr->begin + size;
}
static inline void cptr_inc(Cptr *ptr, int n)
{
    ptr->ptr += n;
    if (ptr->ptr >= ptr->end)
    {
        int offset = ptr->ptr - ptr->end;
        ptr->ptr = ptr->begin + offset;
    }
}

static inline void cptr_copy_into(Cptr *ptr, const void *_src, int size)
{
    int remain = ptr->end - ptr->ptr;
    const uint8_t *src = (const uint8_t *)_src;

    if (remain >= size)
    {
        memcpy(ptr->ptr, src, size);
        cptr_inc(ptr, size);
    }
    else
    {
        memcpy(ptr->ptr, src, remain);
        cptr_inc(ptr, remain);
        memcpy(ptr->ptr, src + remain, size - remain);
        cptr_inc(ptr, size - remain);
    }
}

static inline void cptr_copy_from(Cptr *ptr, void *_dst, int size)
{
    int remain = ptr->end - ptr->ptr;
    uint8_t *dst = (uint8_t *)_dst;

    if (remain >= size)
    {
        memcpy(dst, ptr->ptr, size);
        cptr_inc(ptr, size);
    }
    else
    {
        memcpy(dst, ptr->ptr, remain);
        cptr_inc(ptr, remain);
        memcpy(dst + remain, ptr->ptr, size - remain);
        cptr_inc(ptr, size - remain);
    }
}
