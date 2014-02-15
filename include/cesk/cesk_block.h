#ifndef __CESK_BLOCK_H__
#define __CESK_BLOCK_H__
/* The Block Graph Analyzer
 * Based on the block seperated by dalvik_block.c, 
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

typedef struct {
    const dalvik_block_t* block;      /* the code block */
    cesk_frame_t*   input;      /* input frame */
    cesk_frame_t*   output[0];  /* output frames, contains block->nbranches possible branch */
} cesk_block_t;     /* a node for a block in analysis */

/* build a new analyzer block graph coresponding to the code block graph */
cesk_block_t* cesk_block_graph_new(const dalvik_block_t* entry);

/* analysis one block, 1 means the output changed, 0 means unchangede, -1 indicates errors */
int cesk_block_analysis(cesk_block_t* block);

#endif
