/**
 * @file ceks_diff.h
 * @brief the data structure and utils that used for 
 *        tracking the modification of the frame
 **/
#ifndef __CESK_DIFF_H__
#define __CESK_DIFF_H__
typedef struct _cesk_diff_item_t cesk_diff_item_t;
#include <adam.h>
#include <cesk/cesk_frame.h>
#include <dalvik/dalvik_instruction.h>
/*
 * TODO: a) a data structure to repr the diff between machine states 
 *       b) a method to merge two diffs
 *		 c) function to apply a diff to the store
 *		 d) keep an object list which needs to relocate in each store
 *		 e) reverse diff
 *
 *		 Problem: reverse diff behavious for new object(this is not a problem, because before
 *		          the diff operates the frame, the object must be converted to the store address)
 */

/** @brief type of frame diff items */
enum {
	CESK_DIFF_OPCODE_REUSE,
	CESK_DIFF_OPCODE_SET
};

/** @brief type of frame diff object addressing */
enum {
	CESK_DIFF_FIXED,
	CESK_DIFF_RELOCATED
};

/** @brief an value to patch */
typedef struct {
	union {
		uint32_t      id;  /*!< the value id, if it's a relocated value */
		cesk_value_t* ptr; /*!< the value ptr, if it's a fixed value */
	} value;					/*!< the value */
} cesk_diff_value_t;

/** @brief an patch items */ 
struct _cesk_diff_item_t {
	uint8_t opcode:1;			/*!< the type of operation */
	uint8_t addr_type:1;		/*!< the address type, fixed or relocated */
	uint8_t val_type:1;			/*!< the value type, fiexed or relocated */
	uint32_t addr;				/*!< the affected address, if it's CESK_STORE_ADDR_NULL in memory, that means, we should allocate an address for it */
	union{
		uint32_t 		id;		/*!< the value id, if the object is located in the relocation table, aka RELOCATED */
		cesk_value_t* 	ptr;	/*!< the object pointer, if it's in the store, aka FIXED */
	}value;    					/*!< the value to be operated */
	struct _cesk_diff_item_t* next; /*!< next item in the list */
	struct _cesk_diff_item_t* prev; /*!< previous item in the list */
};

/** @brief reprs an diff */
typedef struct {
	uint8_t	reduced;        /*!< flag indicate if the diff is reduced, before reduced, the item is sorted by time, but after that the 
							     items would be sorted by pair <address, time> */
	cesk_diff_item_t* head; /*!< the content head */
	cesk_diff_item_t* tail; /*!< the content tail */
} cesk_diff_t;
/**
 * @brief initlize the diff pool
 * @return the result of initialization < 0 for error
 **/
int cesk_diff_init();
/**
 * @brief finalize the diff pool
 * @return the return of finalization < 0 for error
 **/
int cesk_diff_finalize();

/** 
 * @brief make an empty diff
 * @return the pointer to the new diff
 **/
cesk_diff_t* cesk_diff_new();

/**
 * @brief dispose a diff 
 * @param diff the diff
 * @return nothing
 **/
void cesk_diff_free(cesk_diff_t* diff);

/**
 * @brief allocate a new memory for a cesk diff item 
 * @return nothing
 **/
cesk_diff_item_t* cesk_diff_item_new();

/**
 * @brief free a diff item
 * @param item the item
 * @return nothing
 **/
void cesk_diff_item_free(cesk_diff_item_t* item);

/** 
 * @brief push next diff item to end of the diff, this is used for gerneating diff
 * @param diff the diff
 * @param item the next item we want to push
 * @return < 0 for error
 **/
int cesk_diff_push_back(cesk_diff_t* diff, cesk_diff_item_t* item);

/**
 * @brief push the next diff item to the begining of the diff list, this is used for generating the reverse of diff
 * @param diff the diff
 * @param item the item to insert
 * @return < 0 for error
 **/
int cesk_diff_push_front(cesk_diff_t* diff, cesk_diff_item_t* item);

/** 
 * @brief reduce the diff e.g. {set 1 to 2, set 1 to 3} ==> {set 1 to 3} 
 * @param diff the diff to reduce, the reduced list is sorted by <address, time>
 * @return < 0 for error
 **/
int cesk_diff_reduce(cesk_diff_t* diff);

/**
 * @brief merge two diff, notice that the merge operator is not commutative, that is merge(A,B) != merge(B,A)
 * @param first the first diff
 * @param second the second diff
 **/
cesk_diff_t* cesk_diff_merge(cesk_diff_t* first, cesk_diff_t* second);

/**
 * @brief apply the diff to the frame
 * @param frame the frame to operate
 * @param diff 
 * @return <0 for error
 **/
int cesk_diff_apply(cesk_frame_t* frame, cesk_diff_t* diff);
#endif
