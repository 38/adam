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

/** @brief merge two set. dest := dest + sour 
 * @param dest the destination set
 * @param sour the source set
 * @return the result of the operation
 */
int cesk_set_merge(cesk_set_t* dest, const cesk_set_t* sour); 
/** @brief push a signle element to the set. dest := dest + {addr} 
 * @param dest the destination set
 * @param addr the source set
 * @return the result of the operation
 */
int cesk_set_push(cesk_set_t* dest, uint32_t addr);
/**
 * @brief modify an element of the set, if the element do not exist, just return
 * @param dest the destination set
 * @param from modify the address from this address
 * @param to to this address
 * @return result of the operation, < 0 when error 
 **/
int cesk_set_modify(cesk_set_t* dest, uint32_t from, uint32_t to);

/** @brief return a iterator that used for tranverse the set 
 *  @param set the set
 *  @param buf the buffer used for creating a new iterator
 *  @return a pointer to the new iterator, which actually equals to buf. 
 *  		NULL indicates error
 */
cesk_set_iter_t* cesk_set_iter(const cesk_set_t* set, cesk_set_iter_t* buf);

/** @brief advance the iterator, the value is the next address in the set
 * 	  	   If the function returns CESK_STORE_ADDR_NULL, that means there's
 * 		   no more element in the set.
 *  @param iter the iterator
 *  @return the next address in the set. CESK_STORE_ADDR_NULL indicates 
 *  		the iterator reach the end of the set
 */
uint32_t cesk_set_iter_next(cesk_set_iter_t* iter);

/** @brief return the size of the set 
 *  @param set the set
 *  @return the number of elements in the set
 */
size_t cesk_set_size(const cesk_set_t* set);

/** @brief duplicate a set, set is a copy-on-write object
 *  @param sour the source set
 *  @return the copy of the source set
 */
cesk_set_t* cesk_set_fork(const cesk_set_t* sour);
/** @brief despose a set 
 *  @param set the set
 *  @return nothing
 */
void cesk_set_free(cesk_set_t* set);

/** @brief check if the set contains some element 
 *  @param set the set
 *  @param addr address to test
 *  @return the result 
 */
int cesk_set_contain(const cesk_set_t* set, uint32_t addr);

/** @brief initialize 
 *  @return < 0 for error
 */
int cesk_set_init();
/** @brief finalize 
 *  @return nothing
 */
void cesk_set_finalize();

/** @brief hash code of two set 
 *  @param set the set
 *  @return hash code
 */
hashval_t cesk_set_hashcode(const cesk_set_t* set);

/** @brief compare two set 
 *  @param first the first set
 *  @param second the second set
 *  @return the result
 */
int cesk_set_equal(const cesk_set_t* first, const cesk_set_t* second);

/** @brief compute a hashcode without incremental style, only for debug porpuse 
 *  @param set the set
 *  @return hash code
 */
hashval_t cesk_set_compute_hashcode(const cesk_set_t* set);

/**
 * @brief convert the set to string
 * @param set
 * @param buffer if it's NULL means use the default buffer
 * @param size the buffer size
 * @return the result string NULL indicates error
 **/
const char* cesk_set_to_string(const cesk_set_t* set, char* buffer, int size);

#endif /* __CESK_SET_H__ */
