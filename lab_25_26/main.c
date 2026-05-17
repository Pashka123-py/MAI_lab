#include <stdio.h>
#include "udt.h"
#include "sort.h"

int main(void)
{
    udt a;
    udt b;

    udt_create(&a);
    udt_create(&b);

    udt_push_back(&a, (data_type){5, 100});
    udt_push_back(&a, (data_type){2, 200});
    udt_push_back(&a, (data_type){8, 300});

    udt_push_back(&b, (data_type){1, 400});
    udt_push_back(&b, (data_type){7, 500});
    udt_push_back(&b, (data_type){3, 600});

    printf("First deque:\n");
    udt_print(&a);

    printf("Second deque:\n");
    udt_print(&b);

    printf("Concatenation:\n");
    udt_concat(&a, &b);
    udt_print(&a);

    printf("Quick sort:\n");

    quick_sort(&a, 0, (int)udt_size(&a) - 1);

    udt_print(&a);

    return 0;
}
