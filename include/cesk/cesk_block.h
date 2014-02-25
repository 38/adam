#ifndef __CESK_BLOCK_H__
#define __CESK_BLOCK_H__
/**@file cesk_block.h
 *
 * @brief The Block Graph Analyzer
 *
 * @details Based on the block seperated by dalvik_block.c, 
 * We can perform a block graph analysis. 
 * At the begnning of the analysis process, all frames remains 
 * empty. For each iteration, we analysis the code blocks and 
 * if all frame remains unchanged after one iteration, 
 * We find a fix point. Then we return the result
 */
#include <constants.h>
#include <dalvik/dalvik_block.h>
#include <dalvik/dalvik_instruction.h>
#include <cesk/cesk_frame.h>

/** @brief data type for a block in block graph */
typedef struct {
    const dalvik_block_t* code_block;      /*!<the code block */
    cesk_frame_t*   input;      /*!<input frame */
    cesk_frame_t*   fanout[0];  /*!<output frames, contains block->nbranches possible branch */
} cesk_block_t;

/** @brief build a new analyzer block graph coresponding to the code block graph 
 * @param entry the entry code block of the function
 * @return the analysis block graph build from the code block graph. NULL indicates error 
 */
cesk_block_t* cesk_block_graph_new(const dalvik_block_t* entry);

/** @brief interpret a signle block 
 *  @param  block  
 *  @return the output frame
 */
cesk_frame_t* cesk_block_interpret(cesk_block_t* block);

#endif
