/**
 * @file cesk_diff.h
 * @brief the data structure and utils that used for 
 *        tracking the modification of the frame
 **/
#ifndef __CESK_DIFF_H__
#define __CESK_DIFF_H__
/** @brief a diff package */
typedef struct _cesk_diff_t cesk_diff_t;
/** @brief an item in the diff package */
typedef struct _cesk_diff_item_t cesk_diff_item_t;

#include <cesk/cesk_value.h>
/**
 * @brief type of a frame address
 **/
enum {
	CESK_DIFF_FRAME_STORE,		/*!< refer an address in store */
	CESK_DIFF_FRAME_REGISTER	/*!< refer a register */
};
/**
 * @brief address in a frame (note: uint32_t is an address in a store, but we also
 *        wants to refer registers here)
 **/
typedef struct {
	uint8_t  type;	/*!< type of this address store address or register number */
	uint32_t value; /*!< the actual store address or register number */
} cesk_diff_frame_addr_t;

/** 
 * @brief type of diff items
 * @note we do not need a type for modifying a field of an object, because the NEW instruction
 *       creates empty value set for each field, that means the address of the field of the new
 *       object is determined at the time the object is allocated. Therefore, the address of the
 *       object field won't change after the object has been created. So there's no need for modifying
 *       the content of the object itself.
 *       The reason why we do not need a reverse operation for allocation-reuse is because all reversed
 *       diff package are input-output diff. In a single block, it's impossible to allocate the object
 *       twice (because same address is always allocated by the same instruction)
 * @todo create empty value set during the creation of new objects 
 **/
enum {
	CESK_DIFF_ITEM_TYPE_NOP,        /*!< an empty diff, can not be a part of diff package, only as one possible result of reduction, 
	                                 *   should be eliminated when reduction completed */
	CESK_DIFF_ITEM_TYPE_ALLOCATE,   /*!< initialize a new address */
	CESK_DIFF_ITEM_TYPE_DEALLOCATE, /*!< finalize a new address, this action should not be present in the result, just used in the reverse diff */
	CESK_DIFF_ITEM_TYPE_REUSE,      /*!< set the reuse flag of this address */
	CESK_DIFF_ITEM_TYPE_ALLOCREUSE, /*!< allocate & reuse */
	CESK_DIFF_ITEM_TYPE_SET,        /*!< modifiy the set in a given address */
	CESK_DIFF_ITEM_TYPE_ALLOCSET,   /*!< allocate a new object and then set a new value */
};

/** 
 * @brief defination of cesk_diff_item_t 
 **/
struct _cesk_diff_item_t {
	uint8_t type;                     /*!< the type of this diff item */
	cesk_diff_frame_addr_t frame_addr;/*!< the frame address affected */
	union{
		uint8_t reuse:1;          /*!< the new value of reuse bit */
		cesk_value_t* value;      /*!< the new value of a set */
	} data;                           /*!< the diff data */
	cesk_diff_item_t* next;           /*!< the next item (used in linked lists)*/
};

/**
 * @brief a diff package
 **/
struct _cesk_diff_t {
	uint8_t           red:1;          /*!< the flag indicates that wether or not the diff package is reduced */
	cesk_diff_item_t* head;           /*!< head of the diff list */
	cesk_diff_item_t* tail;           /*!< tail of the diff list */
};
/**
 * @brief initialize this module
 * @return < 0 for error
 **/
int cesk_diff_init();

/**
 * @brief finalize this module
 * @return nothing
 **/
void cesk_diff_finalize();

/**
 * @brief allocate a new diff item 
 * @return the pointer to newly created diff item, NULL for error
 **/
cesk_diff_item_t* cesk_diff_item_new();

/**
 * @brief free the memory used by the given diff item
 * @param item the diff item to be delocated
 * @return nothing
 **/
void cesk_diff_item_free(cesk_diff_item_t* item);

/**
 * @brief allocate an empty diff package
 * @return the pointer to the empty diff package
 **/
cesk_diff_t* cesk_diff_new();

/**
 * @brief free the memory used by the given diff package
 * @param diff the diff package to be delocated
 * @return nothing
 **/
void cesk_diff_free(cesk_diff_t* diff);

/**
 * @brief append a new item to head of the diff list
 * @param diff the diff package
 * @param item the diff item to be added
 * @return the result of the operation, < 0 means error
 **/
int cesk_diff_push_front(cesk_diff_t* diff, cesk_diff_item_t* item);

/**
 * @brief append a new item to tail of the diff list
 * @param diff the diff package
 * @param item the diff item to be added
 * @return the result of the operation, < 0 means error
 **/
int cesk_diff_push_back(cesk_diff_t* diff, cesk_diff_item_t* item);

/**
 * @brief reduce the diff
 * @note  you can not use the diff before reduction, because reduction might modify the structure of the package
 * @param diff the diff to reduce
 * @return the result of operation, < 0 means error
 **/
int cesk_diff_reduce(cesk_diff_t* diff);

/**
 * @brief deep copy the diff package
 * @param diff the diff package to copy
 * @return the new copy of this diff package NULL for error
 **/
cesk_diff_t* cesk_diff_clone(const cesk_diff_t* diff);

/**
 * @brief apply left diff package to the right diff package (left * right)
 * @param left the left diff package
 * @param right the right diff package
 * @return merge result < 0 means error
 * @note   the diff `right' will be freed after the application, and `left' will be modified
 *         so that it will be the diff package that has been merged so if you will use either
 *         of two input diff, you must provide a copy of the diff package using cesk_diff_clone
 **/
int cesk_diff_apply_d(cesk_diff_t* left, cesk_diff_t* right);

/**
 * @brief apply the diff package to the store
 * @param diff the diff pacakge to be applied
 * @param store the store to apply
 * @return the result of the operation, < 0 means error 
 **/
int cesk_diff_apply_s(const cesk_diff_t* diff, cesk_store_t* store);

/**
 * @brief merge the left diff pacakge and the right diff package 
 * @param left the left diff package
 * @param right the right diff pacakge
 * @return result < 0 means error
 * @note the
 **/
int cesk_diff_merge(cesk_diff_t* left, cesk_diff_t* right);

/**
 * @brief find a diff package that convert sum of products to product of sum
 * 	      that is to find @f$ f = factorize(d_0, S_0, S_1, ... S_N) @f$ such that
 *           @f[ S_0 * d_0 + \sum_{i = 1} S_i = f * (d_0 + \sum{i=1} S_i*d_i) @f]
 *           by using this trick, we can manipulate the diff which is affected by multiply inputs
 *  @param d0 the diff  d0
 *  @param S0 the store S0
 *  @param ... the remaining store, ends with a NULL pointer
 *  @return the result @f$f@f$, NULL means error
 **/
cesk_diff_t* cesk_diff_factorize(const cesk_diff_t* d0, const cesk_store_t* S0, ...);

/**
 * @brief convert a diff item object to string
 * @param item diff item
 * @param buf  the output buffer, if the buffer is NULL, use the defualt buffer
 * @return the result string, NULL indicates errrors
 **/
const char* cesk_diff_item_to_string(const cesk_diff_item_t* item, char* buf);
#endif
