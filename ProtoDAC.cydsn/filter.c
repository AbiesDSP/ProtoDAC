#include "filter.h"

void filter_init(Filter *f, FilterProcess process)
{
    f->process = process;
}

void filter_chain_init(FilterChain *chain, float *proc_buf)
{
    chain->proc_buf = proc_buf;
}

void filter_chain_append(FilterChain *chain, const Filter *f)
{
}

void filter_chain_process(FilterChain *chain, float *src, float *dst, int n)
{
}
