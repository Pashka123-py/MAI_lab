#include "udt.h"

static void swap(data_type *a, data_type *b)
{
    data_type temp = *a;
    *a = *b;
    *b = temp;
}

static int partition(udt *d, int low, int high)
{
    int pivot = d->data[(low + high) / 2].key;

    int i = low;
    int j = high;

    while (i <= j)
    {
        while (d->data[i].key < pivot)
            i++;

        while (d->data[j].key > pivot)
            j--;

        if (i <= j)
        {
            swap(&d->data[i], &d->data[j]);
            i++;
            j--;
        }
    }

    return i;
}

void quick_sort(udt *d, int low, int high)
{
    if (low >= high)
        return;

    int index = partition(d, low, high);

    quick_sort(d, low, index - 1);
    quick_sort(d, index, high);
}
