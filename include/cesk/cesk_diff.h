/**
 * @file cesk_diff.h
 * @brief the data structure and utils that used for 
 *        tracking the modification of the frame
 **/
#ifndef __CESK_DIFF_H__
#define __CESK_DIFF_H__
/** @brief a diff package */
typedef struct _cesk_diff_t cesk_diff_t;
typedef struct _cesk_diff_buffer_t cesk_diff_buffer_t;
#include <vector.h>
#include <const_assertion.h>

#include <cesk/cesk_value.h>
#include <cesk/cesk_frame.h>

/**
 * @brief convert the reuse value to a value pointer
 * @param val the value to convert
 * @return the converted pointer
 **/
#define CESK_DIFF_REUSE_VALUE(val) ((void*)val)
/**
 * @brief define the segmentations in a diff pacakge 
 **/
enum{
	CESK_DIFF_ALLOC,   /*!<Allocate a new object in relocated address [allocate reloc_addr]*/
	CESK_DIFF_REUSE,   /*!<Set the reuse flag, we can use the value pointer as a interger value, and this
	                       will be emitted when an allocation instruction cause the reuse bit changes. 
	                       In this situation, we do not need this alloc record any more, we use an reuse
	                       record to show that this is not the first time we allocate the object */
	CESK_DIFF_REG,     /*!<Set a value to a register [set-reg reg-id new-value] */
	CESK_DIFF_STORE,   /*!<Set a value to a store cell [set-store address new-value] */
	CESK_DIFF_DEALLOC, /*!<Deallocate the object (must not apply to a store) [deallocate reloc_addr] */
	CESK_DIFF_NTYPES   /*!<Number of segmentations in a diff */
};

/* Diff Buffer */

/** 
 * @brief this object is used to store the diff items temporarily,
 *        after finish interpret the block, we can call 
 *        cesk_diff_from_buffer to make a cesk_diff_t type
 **/
struct _cesk_diff_buffer_t{
	int converted;   /*!< indicates how many items are converted */
	vector_t* buffer;/*!< the actual buffer */
	uint8_t reverse; /*!< this buffer is in reverse time order */
};

/**
 * @brief a diff record in a diff package 
 **/
typedef struct _cesk_diff_rec_t{
	uint32_t addr;   /*!< address that this record operates */
	union{
		cesk_value_t* value;    /*!< the cesk_value_t type argument (used in allocation section and store section) */
		cesk_set_t*   set;      /*!< the cesk_set_t type argument   (used in register section) */
		uint8_t       boolean;  /*!< the boolean argument      (used in reuse section) */
		void*         generic;  /*!< the generic pointer */
	} arg;           /*!< the argument of this records */
} cesk_diff_rec_t;
/**
 * @brief the diff package
 * @note the diff items is sorted so that we can merge it by a signle scan
 **/
struct _cesk_diff_t{
	int _index;                         /*!< the index of current element, used for heap manipulation */ 
	int offset[CESK_DIFF_NTYPES + 1];   /*!< the size of each segment */
	cesk_diff_rec_t data[0];          /*!< the data section */
};
CONST_ASSERTION_LAST(cesk_diff_t, data);
CONST_ASSERTION_SIZE(cesk_diff_t, data, 0);

/**
 * @brief create a new diff buffer 
 * @return the created diff buffer, NULL indicates an error
 **/
cesk_diff_buffer_t* cesk_diff_buffer_new(uint8_t reverse);
/**
 * @brief free the memory for diff buffer
 * @return nothing
 **/
void cesk_diff_buffer_free(cesk_diff_buffer_t* buffer);
/**
 * @brief append a diff record to the buffer
 * @param buffer the diff buffer
 * @param type 
 * @param addr the address that this item operates
 * @param value the value if there's one
 * @return < 0 indicates an error
 **/
int cesk_diff_buffer_append(cesk_diff_buffer_t* buffer,int type, uint32_t addr, void* value);
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
 * @brief make a new diff that is the result to apply N input diff into one
 * @param N the number of diffs that want to apply
 * @param args the input cesk_diff_t array
 * @return the newly create diff, NULL indicates error
 */
cesk_diff_t* cesk_diff_apply(int N, cesk_diff_t** args);

/**
 * @brief factorize the sum expression
 * @details make D1*S1 + ... + Dn*Sn = D * (S1 + S2 + ... + Sn) 
 * @param N the number of terms
 * @param current_frame Sn*Dn
 * @param diffs Dn
 * @return the result diff
 **/
cesk_diff_t* cesk_diff_factorize(int N, cesk_diff_t** diffs, const cesk_frame_t** current_frame);
/**
 * @brief convert the diff package to a human-reable string
 * @param diff the diff pakcage
 * @param buf the buffer if buf == NULL, use the default buf
 * @param size the size of buffer
 * @return the result string, NULL indicates error
 **/
const char* cesk_diff_to_string(const cesk_diff_t* diff, char* buf, int size);
/**
 * @brief create a empty diff package
 * @return the newly created empty diff, NULL indicates an error
 */
cesk_diff_t* cesk_diff_empty();
#endif
