/*
 * pq.c
 */

#include <malloc.h>
#include <stdint.h>

#include "pq.h"

static void min_swap(pq *p, int i, int j);
static void min_heapify(pq *p, int i);
static void min_trickle_up(pq *p, int i);

pq *pq_create(int capacity)
{
    pq *p;

    p = (pq *) malloc(sizeof(pq));
    if(!p)
        return NULL;

    p->keys = (int64_t *) calloc(capacity, sizeof(int64_t));
    p->data = calloc(capacity, sizeof(void *));

    p->occupancy = 0;
    p->capacity = capacity;

    return p;
}

void pq_destroy(pq *p)
{
    if(!p)
        return;

    free(p->keys);
    free(p->data);
}

void pq_print(pq *p) {
    int i;

    if(!p)
        return;

    printf("pq> contents:\n");
    for(i=0; i<p->occupancy; i++) {
        printf("pq>   %d: %016lx (%ld)\n", i, *(p->keys + i), *(p->keys + i));
    }
}

int pq_is_valid(pq *p) {
    int i, left, right;

    if(!p)
        return 0;

    for(i=0; i<p->occupancy; i++) {
        left = i * 2 + 1;
        right = i * 2 + 2;

        if(*(p->keys + left) < *(p->keys + i)
                || *(p->keys + right) < *(p->keys + i)) {
            printf("pq>>>>> children of %d fail\n", i);
            return 0;
        }
    }

    return 1;
}

int pq_insert(pq *p, int64_t key, void *data)
{
    if(!p || p->occupancy >= p->capacity)
        return -1;

    *(p->keys + p->occupancy) = key;
    *(p->data + p->occupancy) = data;

    p->occupancy++;

    min_trickle_up(p, p->occupancy - 1);

    /*
    printf("pq> inserted item with key %016lx (%ld)\n", key, key);
    if(!pq_is_valid(p))
        pq_print(p);
    */

    return p->occupancy;
}

void *pq_pop(pq *p, int64_t *rkey)
{
    if(!p || p->occupancy <= 0)
        return NULL;

    min_swap(p, 0, p->occupancy - 1);
    p->occupancy--;
    min_heapify(p, 0);

    if(rkey)
        *rkey = *(p->keys + p->occupancy);

    /*
    printf("pq> popped item with key %016lx (%ld)\n",
            *(p->keys + p->occupancy), *(p->keys + p->occupancy));
    if(!pq_is_valid(p))
        pq_print(p);
    */

    return *(p->data + p->occupancy);
}

static void min_swap(pq *p, int i, int j)
{
    int64_t tempkey;
    void *tempdata;

    tempkey = *(p->keys + i);
    *(p->keys + i) = *(p->keys + j);
    *(p->keys + j) = tempkey;

    tempdata = *(p->data + i);
    *(p->data + i) = *(p->data + j);
    *(p->data + j) = tempdata;
}

static void min_heapify(pq *p, int i)
{
    int left, right, min;

    left = i * 2 + 1;
    right = i * 2 + 2;

    if((left < p->occupancy) && (*(p->keys + left) < *(p->keys + i)))
        min = left;
    else
        min = i;

    if((right < p->occupancy) && (*(p->keys + right) < *(p->keys + min)))
        min = right;

    if(min != i) {
        min_swap(p, i, min);
        min_heapify(p, min);
    }
}

static void min_trickle_up(pq *p, int i)
{
    int parent_index;

    parent_index = (i - 1) / 2;

    if((i > 0) && (*(p->keys + parent_index) > *(p->keys + i))) {
        min_swap(p, parent_index, i);
        min_trickle_up(p, parent_index);
    }
}

