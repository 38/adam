/**
 * @brief the Block Analyzer
 **/
#ifndef __CESK_BLOCK_H__
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
 * @brief initialize this module
 * @return < 0 if failed to initialize
 **/
int cesk_block_init();
/**
 * @brief fianlize this module
 * @return nothing
 **/
void cesk_block_finalize();

/**
 * @brief Analyze a code block, return the result in buf
 * @param code the code block to analyze
 * @param frame the stack frame
 * @param rtab  the relocation table 
 * @param buf the output buffer
 * @param caller_ctx the caller context
 * @return < 0 indicates erorrs
 **/
int cesk_block_analyze(
		const dalvik_block_t* code, 
		cesk_frame_t* frame, 
		cesk_reloc_table_t* rtab, 
		cesk_block_result_t* buf, 
		const void* caller_ctx);
#endif
