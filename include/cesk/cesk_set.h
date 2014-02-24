#ifndef __CESK_SET_H__
#define __CESK_SET_H__
#include <constants.h>
#include <log.h>
/** @file  cesk_set.h
 *  @brief the address used for CESK Virtual Machine 
 */
/* this file need previous definations */
/** @brief the node in the hash table */
typedef struct _cesk_set_node_t cesk_set_node_t;
/** @brief the data structure for the set */
typedef struct _cesk_set_t cesk_set_t;
/** @brief the data structure for the iterator */
typedef struct _cesk_set_iter_t cesk_set_iter_t;

#include <cesk/cesk_set.h>
#include <cesk/cesk_store.h>

struct _cesk_set_iter_t{
    cesk_set_node_t *next;
};

/** @brief Create an empty set 
 *  @return the new set
 */
cesk_set_t* cesk_set_empty_set();

/* @brief join two set. dest := dest + sour 
 * @param dest the destination set
 * @param sour the source set
 * @return the result of the operation
 */
int cesk_set_join(cesk_set_t* dest, cesk_set_t* sour); 
/* push a signle element to the set. dest := dest + {addr} */
int cesk_set_push(cesk_set_t* dest, uint32_t addr);

/* return a iterator that used for tranverse the set */
cesk_set_iter_t* cesk_set_iter(cesk_set_t* set, cesk_set_iter_t* buf);

/* advance the iterator, the value is the next address in the set
 * If the function returns CESK_STORE_ADDR_NULL, that means there's
 * no more element in the set.
 */
uint32_t cesk_set_iter_next(cesk_set_iter_t* iter);

/* return the size of the set */
size_t cesk_set_size(cesk_set_t* set);

/* fork a set */
cesk_set_t* cesk_set_fork(cesk_set_t* sour);
/* despose a set */
void cesk_set_free(cesk_set_t* set);

/* check if the set contains some element */
int cesk_set_contain(cesk_set_t* set, uint32_t addr);

/* init & finalize */
void cesk_set_init();
void cesk_set_finalize();

/* hash code of two set */
hashval_t cesk_set_hashcode(cesk_set_t* set);

/* compare two set */
int cesk_set_equal(cesk_set_t* first, cesk_set_t* second);

/* compute a hashcode without incremental style, only for debug porpuse */
hashval_t cesk_set_compute_hashcode(cesk_set_t* set);

#endif /* __CESK_SET_H__ */
