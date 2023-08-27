#pragma once
#include "list2.h"
#include <stdint.h>

typedef void (*FilterProcess)(const float *src, float *dst, int n);

typedef struct Filter
{
    FilterProcess process;
} Filter;

typedef struct FilterChain
{
    List filters;
    float *proc_buf;
} FilterChain;

void filter_init(Filter *f, FilterProcess process);

//
void filter_chain_init(FilterChain *chain, float *proc_buf);
void filter_chain_append(FilterChain *chain, const Filter *f);

void filter_chain_process(FilterChain *chain, float *src, float *dst, int n);
