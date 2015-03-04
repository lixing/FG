/*
 * pq.h
 */

#ifndef __PQ_H_
#define __PQ_H_

#include <stdint.h>

typedef struct pq_ pq;

struct pq_ {
    int occupancy;
    int capacity;
    int64_t *keys;
    void **data;
};

pq *pq_create(int capacity);
void pq_destroy(pq *p);
int pq_insert(pq *p, int64_t key, void *data);
void *pq_pop(pq *p, int64_t *rkey);

#endif /* __PQ_H_ */

