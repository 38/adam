/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @todo review the code, and the idea of diff
 **/
#include <const_assertion.h>
#include <cesk/cesk_diff.h>
#include <vector.h>
/**
 * @brief define the segmentations in a diff pacakge 
 **/
enum{
	CESK_DIFF_ALLOC,
	CESK_DIFF_REUSE,
	CESK_DIFF_REG,
	CESK_DIFF_STORE,
	CESK_DIFF_DEALLOC,
	CESK_DIFF_NTYPES
};
typedef struct _cesk_diff_segment_node_t _cesk_diff_segment_node_t;
typedef struct _cesk_diff_segment_t _cesk_diff_segment_t;
/**
 * @brief a node in the diff segment
 **/
struct _cesk_diff_segment_node_t{
	// if we can oganize the address sorted, we can merge them fast */
	int addr;   /*!< the address to operate, in the Register Segment, this means the register number */
	union{
		cesk_value_t* cesk_value;  /*!< this is a cesk_value_t */
		uint32_t      uint32;      /*!< this is a uint32_t type */  
	} value;    /*!< the value to set, if it's in the reuse section, value means the new reuse flag */
};
/**
 * @brief the diff package
 **/
struct _cesk_diff_segment_t{
	int size[CESK_DIFF_NTYPES];   /*!< the size of each segment */
	_cesk_diff_segment_node_t data[0]; /*!< the data section */
};
CONST_ASSERTION_LAST(_cesk_diff_segment_t, data);
CONST_ASSERTION_SIZE(_cesk_diff_segment_t, data, 0);
