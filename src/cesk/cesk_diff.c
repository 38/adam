/**
 * @file cesk_diff.c
 * @brief implementation of frame diff package
 * @todo review the code, and the idea of diff
 **/
#include <const_assertion.h>
#include <vector.h>

#include <cesk/cesk_diff.h>
#include <cesk/cesk_value.h>
/* previous defs */
typedef struct _cesk_diff_segment_node_t _cesk_diff_segment_node_t;
typedef struct _cesk_diff_segment_t _cesk_diff_segment_t;
/**
 * @brief a node in the diff segment
 **/
struct _cesk_diff_segment_node_t{
	cesk_value_t type; /*!< which type this diff item is, useful when sorting */
	int addr;   /*!< the address to operate, in the Register Segment, this means the register number */
};
/**
 * @brief the diff package
 * @note the diff items is sorted so that we can merge it by a signle scan
 **/
struct _cesk_diff_segment_t{
	int size[CESK_DIFF_NTYPES];   /*!< the size of each segment */
	_cesk_diff_segment_node_t data[0]; /*!< the data section */
};
CONST_ASSERTION_LAST(_cesk_diff_segment_t, data);
CONST_ASSERTION_SIZE(_cesk_diff_segment_t, data, 0);


