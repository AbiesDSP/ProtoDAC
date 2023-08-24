extern "C"
{
#include "list.h"
}
#include <catch2/catch_test_macros.hpp>

TEST_CASE("List Basics", "[list]")
{
    List l;

    list_init(&l);

    int x[8] = {0, 1, 2, 3, 4, 5, 6, 7};

    list_append(&l, x);
    REQUIRE(l.size == 1);

    for (int i = 1; i < 8; i++)
    {
        list_append(&l, &x[i]);
    }
    REQUIRE(l.size == 8);

    {
        int i = 0;
        for (ListNode *it = list_iter(&l, 0); it != NULL; it = it->next, i++)
        {
            int *xref = (int *)it->ref;
            REQUIRE(*xref == x[i]);

            xref = (int *)list_index(&l, i);
            REQUIRE(*xref == x[i]);
        }
    }

    list_destroy(&l);
}
