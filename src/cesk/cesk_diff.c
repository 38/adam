/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @todo review the code, and the idea of diff
 **/

#include <cesk/cesk_diff.h>
#include <vector.h>
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
