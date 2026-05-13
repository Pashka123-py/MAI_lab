#include <stdio.h>
#include "udt.h"

void udt_create(udt *d)
{
    d->size = 0;
}

bool udt_is_empty(const udt *d)
{
    return d->size == 0;
}

void udt_push_front(udt *d, const data_type item)
{
    if (d->size >= UDT_CAPACITY)
        return;

    for (size_t i = d->size; i > 0; --i)
        d->data[i] = d->data[i - 1];

    d->data[0] = item;
    d->size++;
}

void udt_push_back(udt *d, const data_type item)
{
    if (d->size >= UDT_CAPACITY)
        return;

    d->data[d->size++] = item;
}

void udt_pop_front(udt *d)
{
    if (udt_is_empty(d))
        return;

    for (size_t i = 0; i < d->size - 1; ++i)
        d->data[i] = d->data[i + 1];

    d->size--;
}

void udt_pop_back(udt *d)
{
    if (udt_is_empty(d))
        return;

    d->size--;
}

void udt_print(const udt *d)
{
    printf("Deque:\n");

    for (size_t i = 0; i < d->size; ++i)
    {
        printf("[%zu] key=%d value=%d\n",
               i,
               d->data[i].key,
               d->data[i].value);
    }

    printf("\n");
}

size_t udt_size(const udt *d)
{
    return d->size;
}

void udt_insert(udt *d, const data_type item)
{
    udt_push_back(d, item);
}

void udt_erase(udt *d, const key_type key)
{
    for (size_t i = 0; i < d->size; ++i)
    {
        if (d->data[i].key == key)
        {
            for (size_t j = i; j < d->size - 1; ++j)
                d->data[j] = d->data[j + 1];

            d->size--;
            return;
        }
    }
}

void udt_concat(udt *a, const udt *b)
{
    for (size_t i = 0; i < b->size; ++i)
    {
        if (a->size >= UDT_CAPACITY)
            return;

        udt_push_back(a, b->data[i]);
    }
}