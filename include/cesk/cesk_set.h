#ifndef __CESK_SET_H__
#define __CESK_SET_H__
#include <constants.h>
#include <log.h>

/* this file need previous definations */
typedef struct _cesk_set_t cesk_set_t;
typedef struct _cesk_set_iter_t cesk_set_iter_t;

#include <cesk/cesk_set.h>
#include <cesk/cesk_store.h>

/* TODO: how to compare two object ? */

/* Create an empty set */
cesk_set_t* cesk_set_empty_set();

/* join two set. dest := dest + sour */
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

/* init & finalize */
void cesk_set_init();
void cesk_set_finalize();

#endif /* __CESK_SET_H__ */
