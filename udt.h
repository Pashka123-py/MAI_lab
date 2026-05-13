#ifndef _UDT_H_
#define _UDT_H_

#include <stdbool.h>
#include <stddef.h>

#define UDT_CAPACITY 100

typedef int key_type;
typedef int value_type;

typedef struct {
    key_type key;
    value_type value;
} data_type;

typedef struct {
    data_type data[UDT_CAPACITY];
    size_t size;
} udt;

void udt_create(udt *);
bool udt_is_empty(const udt *);

void udt_push_front(udt *, const data_type);
void udt_push_back(udt *, const data_type);

void udt_pop_front(udt *);
void udt_pop_back(udt *);

void udt_print(const udt *);
size_t udt_size(const udt *);

void udt_insert(udt *, const data_type);
void udt_erase(udt *, const key_type);

void udt_concat(udt *, const udt *);

#endif