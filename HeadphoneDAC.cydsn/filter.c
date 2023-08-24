#include "filter.h"

void filter_init(Filter *f, FilterProcess process)
{
    f->process = process;
}

void filter_chain_init(FilterChain *chain, float *proc_buf)
{
    list_init(&chain->filters);
    chain->proc_buf = proc_buf;
}

void filter_chain_append(FilterChain *chain, const Filter *f)
{
    list_append(&chain->filters, f);
}

void filter_chain_process(FilterChain *chain, float *src, float *dst, int n)
{
    int remaining = chain->filters.size;
    const int last_filter = remaining - 1;
    Filter *f;

    float *ibuf = src;
    float *obuf = chain->proc_buf;

    // for (list_node *it = chain->filters.begin; it != NULL; it = it->next)
    for (int i = 0; i < remaining; ++i)
    {
        f = list_index(&chain->filters, i);
        if (i == last_filter)
        {
            obuf = dst;
        }

        // Process the data through the filter.
        f->process(ibuf, obuf, n);

        // Alternate between src and proc_buf for processing.
        if (ibuf == src)
        {
            ibuf = chain->proc_buf;
            obuf = src;
        }
        else
        {
            ibuf = src;
            obuf = chain->proc_buf;
        }
    }
}
