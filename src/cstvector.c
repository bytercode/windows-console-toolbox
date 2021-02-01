#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cstvector.h"

int cstvector_init(cstvector_t *vec, size_t elem_size)
{
    vec->data = NULL;
    vec->capacity = 0;
    vec->size = 0;
    vec->elem_size = elem_size;
    return 0;
}

int cstvector_reserve(cstvector_t *vec, size_t newsize)
{
    if (newsize <= vec->capacity)
        return 0;

    void *realloced_data = realloc(vec->data, newsize * vec->elem_size);
    if (realloced_data == NULL)
        return -1;

    vec->data = realloced_data;
    vec->capacity = newsize;
    return 0;
}

int cstvector_nextreserve(cstvector_t *vec)
{
    return cstvector_reserve(vec, vec->capacity == 0 ? 1 : vec->capacity * 2);
}

int cstvector_resize(cstvector_t *vec, size_t newsize, void *newfill)
{
    if (newsize > vec->capacity)
    {
        if (cstvector_reserve(vec, newsize))
            return -1;

        if (newfill != NULL)
        {
            for (size_t i = vec->size; vec->size < newsize; ++i)
                memcpy(vec->data + vec->elem_size * i, newfill, vec->elem_size);
        }
    }
    vec->size = newsize;
    return 0;
}

void cstvector_destroy(cstvector_t *vec)
{
    // It shouldn't do anything if vec->data is null...
    free(vec->data);
}

void *cstvector_back(cstvector_t *vec)
{
    if (vec->size == 0)
        return NULL;

    return vec->data + vec->elem_size * (vec->size-1);
}

void *cstvector_front(cstvector_t *vec)
{
    if (vec->size == 0)
        return NULL;

    return vec->data;
}

int cstvector_push_back(cstvector_t *vec, void *value)
{
    // >= cuz... idk... safety or whatever?
    if (vec->size >= vec->capacity)
        if (cstvector_nextreserve(vec) != 0)
            return -1;

    memcpy(vec->data + (vec->elem_size * vec->size++), value, vec->elem_size);
    return 0;
}

int cstvector_pop_back(cstvector_t *vec)
{
    if (vec->size == 0)
        return -1;

    --vec->size;
    return 0;
}

void *cstvector_getat(cstvector_t *vec, size_t index)
{
    if (index >= vec->size)
        return NULL;

    return vec->data + vec->elem_size * index;
}

int cstvector_setat(cstvector_t *vec, size_t index, void *value)
{
    if (index >= vec->size)
        return -1;

    memcpy(vec->data + vec->elem_size * index, value, vec->elem_size);
    return 0;
}

void cstvector_clear(cstvector_t *vec)
{
    vec->size = 0;
}

int cstvector_insert(cstvector_t *vec, size_t index, void *value)
{
    if (index >= vec->size)
        return -1;

    if (vec->size >= vec->capacity)
        if (cstvector_nextreserve(vec) != 0)
            return -1;

    memmove(vec->data + vec->elem_size * (index + 1), vec->data + vec->elem_size * index, (vec->size - index) * vec->elem_size);
    memcpy(vec->data + vec->elem_size * index, value, vec->elem_size);
    ++vec->size;
    return 0;
}

int cstvector_erase(cstvector_t *vec, size_t index)
{
    if (index >= vec->size)
        return -1;

    memmove(vec->data + vec->elem_size * index, vec->data + vec->elem_size * (index + 1), vec->elem_size * (vec->size - index - 1));
    --vec->size;
    return 0;
}
