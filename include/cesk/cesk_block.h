/**
 * @brief the Block Analyzer
 **/
#ifndef __CEKS_BLOCK_H__
#define __CESK_BLOCK_H__
#include <constants.h>
#include <const_assertion.h>

#include <dalvik/dalvik_block.h>

#include <cesk/cesk_frame.h>
#include <cesk/cesk_diff.h>

/**
 * @brief the block analyzer result
 **/
typedef struct {
	cesk_frame_t* frame;    /*!< the frame */
	cesk_diff_t*  diff;     /*!< the diff */
	cesk_diff_t*  inverse;  /*!< the inversion of diff */
} cesk_block_result_t;
/**
 * @brief Analyze a code block, return the result in buf
 * @param code the code block to analyze
 * @param frame the stack frame
 * @param rtab  the relocation table 
 * @param buf the output buffer
 * @return < 0 indicates erorrs
 **/
int cesk_block_analyze(const dalvik_block_t* code, cesk_frame_t* frame, cesk_reloc_table_t* rtab, cesk_block_result_t* buf);
#endif
