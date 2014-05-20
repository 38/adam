/**
 * @file cesk_diff.h
 * @brief the data structure and utils that used for 
 *        tracking the modification of the frame
 **/
#ifndef __CESK_DIFF_H__
#define __CESK_DIFF_H__
/** @brief a diff package */
typedef struct _cesk_diff_t cesk_diff_t;
#include <vector.h>
#include <const_assertion.h>

#include <cesk/cesk_value.h>
#include <cesk/cesk_frame.h>
/**
 * @brief define the segmentations in a diff pacakge 
 **/
enum{
	CESK_DIFF_ALLOC,   /*!<Allocate a new object in relocated address <allocate reloc_addr>*/
	CESK_DIFF_REUSE,   /*!<Only for Clear the reuse flag, because set reuse flag it done by allocator <clear-reuse reloc_addr> */
	CESK_DIFF_REG,     /*!<Set a value to a register <set-reg reg-id new-value> */
	CESK_DIFF_STORE,   /*!<Set a value to a store cell <set-store address new-value> */
	CESK_DIFF_DEALLOC, /*!<Deallocate the object (must not apply to a store) <deallocate reloc_addr> */
	CESK_DIFF_NTYPES   /*!<Number of segmentations in a diff */
};

/* Diff Buffer */

/** 
 * @brief this object is used to store the diff items temporarily,
 *        after finish interpret the block, we can call 
 *        cesk_diff_from_buffer to make a cesk_diff_t type
 **/
typedef struct {
	int converted;   /*!< indicates how many items are converted */
	vector_t* buffer;/*!< the actual buffer */
} cesk_diff_buffer_t;

/**
 * @brief create a new diff buffer 
 * @return the created diff buffer, NULL indicates an error
 **/
cesk_diff_buffer_t* cesk_diff_buffer_new();
/**
 * @brief free the memory for diff buffer
 * @return nothing
 **/
void cesk_diff_buffer_free(cesk_diff_buffer_t* buffer);
/**
 * @brief add a diff item to the buffer
 * @param buffer the diff buffer
 * @param type 
 * @param addr the address that this item operates
 * @param value the value if there's one
 * @return < 0 indicates an error
 **/
int cesk_diff_buffer_add(cesk_diff_buffer_t* buffer,int type, uint32_t addr, cesk_value_t* value);
/**
 * @brief construct a new cesk_diff_t according to a given diff buffer
 * @param buffer the diff buffer
 * @return the newly created diff, NULL indicates error
 **/
cesk_diff_t* cesk_diff_from_buffer(cesk_diff_buffer_t* buffer);

/* Diff Ops */
/**
 * @brief free the diff 
 * @param diff the diff to free
 * @return nothing
 **/
void cesk_diff_free(cesk_diff_t* diff);

/**
 * @brief make a new diff that is the union of N input diffs
 * @param N the number of diffs that want to merge
 * @return the newly created diff, NULL indicates error
 **/
cesk_diff_t* cesk_diff_union(int N, cesk_diff_t** args);

/**
 * @brief make a new diff that is the result to apply N input diff into one
 * @param N the number of diffs that want to apply
 * @return the newly create diff, NULL indicates error
 */
cesk_diff_t* cesk_diff_apply(int N, cesk_diff_t** args);

/**
 * @brief factorize the sum expression
 * @detail make D1*S1 + ... + Dn*Sn = D * (S1 + S2 + ... + Sn) 
 * @param N the number of terms
 * @note call this using cesk_diff_fractorize(N, diff1, const_sotre1, diff2, const_store2, .....);
 * @return the result diff
 **/
cesk_diff_t* cesk_diff_factorize(int N, cesk_diff_t** diffs, const cesk_frame_t* frames[]);
#endif
