#pragma once
#include <stdint.h>
#include <string.h>

typedef struct cptr
{
    uint8_t *it;
    uint8_t *begin;
    const uint8_t *end;
} cptr;

static inline void cptr_init(cptr *ptr, uint8_t *buf, size_t size)
{
    ptr->it = buf;
    ptr->begin = buf;
    ptr->end = ptr->begin + size;
}
static inline void cptr_inc(cptr *ptr, int n)
{
    ptr->it += n;
    if (ptr->it >= ptr->end)
    {
        int offset = ptr->it - ptr->end;
        ptr->it = ptr->begin + offset;
    }
}

static inline void cptr_copy_into(cptr *ptr, const void *_src, size_t size)
{
    size_t remain = ptr->end - ptr->it;
    const uint8_t *src = (const uint8_t *)_src;

    if (remain >= size)
    {
        memcpy(ptr->it, src, size);
        cptr_inc(ptr, size);
    }
    else
    {
        memcpy(ptr->it, src, remain);
        cptr_inc(ptr, remain);
        memcpy(ptr->it, src + remain, size - remain);
        cptr_inc(ptr, size - remain);
    }
}

static inline void cptr_copy_from(cptr *ptr, void *_dst, size_t size)
{
    size_t remain = ptr->end - ptr->it;
    uint8_t *dst = (uint8_t *)_dst;

    if (remain >= size)
    {
        memcpy(dst, ptr->it, size);
        cptr_inc(ptr, size);
    }
    else
    {
        memcpy(dst, ptr->it, remain);
        cptr_inc(ptr, remain);
        memcpy(dst + remain, ptr->it, size - remain);
        cptr_inc(ptr, size - remain);
    }
}
