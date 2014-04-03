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
 **/
enum {
	CESK_DIFF_ADDR_ALLOCATE,    /*!< initialize a new address */
	CESK_DIFF_ADDR_DELOCATE,    /*!< finalize a new address, this action should not be present in the result, just used in the reverse diff */
	CESK_DIFF_ADDR_REUSE,		/*!< set the reuse flag of this address */
	CESK_DIFF_SET				/*!< modifiy the set in a given address */
};

/** 
 * @brief defination of cesk_diff_item_t 
 **/
struct _cesk_diff_item_t {
	uint8_t type;                     /*!< the type of this diff item */
	cesk_diff_frame_addr_t frame_addr;/*!< the frame address affected */
	union{
		uint8_t reuse:1;			  /*!< the new value of reuse bit */
		cesk_value_t* value;          /*!< the new value of a set */
	} data;                           /*!< the diff data */
	cesk_diff_item_t* next;           /*!< the next item (used in linked lists)*/
};

/**
 * @brief a diff package
 **/
struct _cesk_diff_t {
	cesk_diff_item_t* list;           /*!< the diff list */
};

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
 * @brief append a new item to the diff 
 * @param diff the diff package
 * @param item the diff item to be added
 * @return the result of the operation, < 0 means error
 **/
int cesk_diff_append(cesk_diff_t* diff, cesk_diff_item_t* item);

/**
 * @brief apply left diff package to the right diff package (left * right)
 * @param left the left diff package
 * @param right the right diff package
 * @return the pointer to the result diff package, NULL means error
 **/
cesk_diff_t* cesk_diff_apply_d(const cesk_diff_t* left, const cesk_diff_t* right);

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
 * @return the pointer to the result diff package, NULL means error
 **/
cesk_diff_t* cesk_diff_merge(const cesk_diff_t* left, const cesk_diff_t* right);

/**
 * @brief find a diff package that convert sum of products to product of sum
 * 	      that is to find @f$ f = factorize(d_0, S_0, S_1, ... S_N) @f$ such that
 * 	      @f[ S_0 * d_0 + \sum_{i = 1} S_i = f * (d_0 + \sum{i=1} S_i*d_i) @f]
 * 	      by using this trick, we can manipulate the diff which is affected by multiply inputs
 *  @param d0 the diff  d0
 *  @param S0 the store S0
 *  @param ... the remaining store, ends with a NULL pointer
 *  @return the result @f$f@f$, NULL means error
 **/
cesk_diff_t* cesk_diff_factorize(const cesk_diff_t* d0, const cesk_store_t* S0, ...);
#endif
