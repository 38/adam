#ifndef __CESK_SET_H__
#define __CESK_SET_H__
#include <constants.h>
#include <stdint.h>

typedef struct _cesk_set_t cesk_set_t;
typedef struct _cesk_set_iterator_t cesk_set_iter_t;

/* return an empty set */
cesk_set_t*  cesk_set_empty_set(void);

/* set operations */
int cesk_set_join(cesk_set_t* dest, cesk_set_t* sour);   /* dest = dest join sour */
int cesk_set_push(cesk_set_t* dest, uint32_t addr);      /* dest = dest join {set} */
cesk_set_iter_t* cesk_set_iter(cesk_set_t* set);     /* get an iterator to tranverse the set */

void cesk_set_free(cesk_set_t* set);

cesk_set_t* cesk_set_fork(cesk_set_t* set);              /* fork a cesk set */
#endif
